#include <iostream>
#include <iomanip>
#include <string>
#include <fstream>
#include <unordered_map>
#include <map>
#include <set>
#include <filesystem>
#include <optional>

#ifdef _DEBUG
#define DBOUT std::cout // or any other ostream
#else
#define DBOUT 0 && std::cout
#endif

#include <nlohmann/json.hpp>
#include "StringSplit.hpp"

template <std::size_t n>
class NCharString
{
    std::array<char, n> c = {};
    unsigned char size_;

public:
    NCharString(std::string_view sv)
    {
        std::size_t i = 0;
        for (; i < sv.size() && i < n; i++)
        {
            c[i] = sv.at(i);
        }
        size_ = i;
    }
    std::string_view toStringView() const noexcept
    {
        return std::string_view(c.begin(), c.end());
    }
    std::string toString() const noexcept
    {
        std::string str = "";
        for (uint16_t i = 0; i < size_; i++)
        {
            str += c[i];
        }
        return str;
    }
    std::size_t size() const noexcept
    {
        return (std::size_t)size_;
    }
    char at(std::size_t index) const noexcept
    {
        return c.at(index);
    }
    friend std::ostream &operator<<(std::ostream &os, const NCharString<n> &tcs) noexcept;
};
template <std::size_t n>
std::ostream &operator<<(std::ostream &os, const NCharString<n> &tcs) noexcept
{
    for (unsigned char i = 0; i < tcs.size_; i++)
    {
        os << tcs.c[i];
    }

    return os;
}

int charDigitsToInt(const std::string_view sv) noexcept
{
    // Since we know the input will be a string of an int
    // we can use a simpler function instead of std::atoi
    int val = 0;
    for (const auto c : sv)
    {
        val = val * 10 + (c - '0');
    }
    return val;
}

// Only uses 0 or 2 characters, ISO 639-1 codes
typedef std::string ISOLanguage;

// Only uses 2 characters, ISO 3166-1 Alpha-2 codes
typedef std::string ISOCountryCode;

// It is an integer
typedef int GeoNameId;

// ${CountryCode}.${IDENTIFIER}
typedef std::string Admin1Code;

typedef std::string IANATimezone;

struct LocalizedNames
{
    std::string cityName;
    std::string admin1Name;
    std::string countryName;

    void toStreamAsTxt(std::ostream &os) const noexcept
    {
        os << cityName << '\t' << admin1Name << '\t' << countryName;
    }
};

struct GeoLocation
{
    IANATimezone timezone;
    std::string latitude;
    std::string longitude;

    void toStreamAsTxt(std::ostream &os) const noexcept
    {
        os << timezone << '\t' << latitude << '\t' << longitude;
    }
};

struct Record
{
    GeoNameId gni;
    ISOCountryCode countryCode;
    GeoLocation location;
    LocalizedNames latinizedName;
    std::map<ISOLanguage, LocalizedNames> localizedNames;

    void toStreamAsTxt(std::ostream &os) const noexcept
    {
        os << countryCode << '\t';
        location.toStreamAsTxt(os);
        os << '\t';
        latinizedName.toStreamAsTxt(os);
        os << '\t';
        os << localizedNames.size() << '\t';
        for (const auto &kv : localizedNames)
        {
            os << kv.first << '\t';
            kv.second.toStreamAsTxt(os);
            os << '\t';
        }
    }
};

std::unordered_map<GeoNameId, std::map<ISOLanguage, std::pair<std::string, bool>>> parseAlternateNames(
    const std::string &alternateNamesPath,
    const std::set<ISOLanguage> &SELECTED_LANGUAGES,
    const std::set<ISOCountryCode> &SELECTED_COUNTRIES) noexcept
{
    // GeoNameId -> { ISOLanguage, { alternativeName, isPreferred } }
    std::unordered_map<GeoNameId,
                       std::map<ISOLanguage,
                                std::pair<std::string, bool>>>
        alternateData = {};

    std::ifstream inputFile = std::ifstream(alternateNamesPath);
    std::size_t lineNumber = 0;
    for (std::string line; std::getline(inputFile, line);)
    {
        lineNumber++;
        if (lineNumber % 100'000 == 0)
        {
            std::cout << "Currently on line " << lineNumber << "\n";
        }
        const auto strvs = split<5>(line, '\t');

        const auto languageSV = strvs.at(2);
        const auto languageSize = languageSV.size();

        if (languageSize > 2)
        {
            // The string is not an ISO language
            DBOUT << lineNumber << " Skipping due to not ISO language, received: \"" << languageSV << "\"\n";
            continue;
        }
        const ISOLanguage language = std::string(languageSV); // Will not allocate due to SSO
        if (SELECTED_LANGUAGES.find(language) == SELECTED_LANGUAGES.end())
        {
            // The language is not selected, but is not the default
            DBOUT << lineNumber << " Skipping due to not selected, received: \"" << languageSV << "\"\n";
            continue;
        }

        const GeoNameId geonameid = charDigitsToInt(strvs.at(1));
        const bool isCurrentPreferred = strvs.at(4) == "1";

        auto languageNameMap = alternateData.find(geonameid);
        if (alternateData.find(geonameid) == alternateData.end())
        {
            // The geonameid key doesn't exist, create its map

            DBOUT << lineNumber << " Creating entry for " << geonameid << " since it didn't exist\n";
            const std::string alternativeName = std::string(strvs.at(3));
            alternateData.insert({geonameid, {{language, {alternativeName, isCurrentPreferred}}}});
        }
        else
        {
            // Check if language key already exists
            auto &languageAlternateNameInfo = languageNameMap->second;
            auto alternateNameInfo = languageAlternateNameInfo.find(language);
            if (alternateNameInfo == languageAlternateNameInfo.end())
            {
                // The language has not yet been inserted.
                DBOUT << lineNumber << " Creating entry for " << geonameid << " (" << language << ") since it didn't exist.\n";
                const std::string alternativeName = std::string(strvs.at(3));
                languageAlternateNameInfo.insert({language, {alternativeName, isCurrentPreferred}});
            }
            else
            {
                auto &nameInfo = alternateNameInfo->second;
                if (!nameInfo.second && isCurrentPreferred)
                {
                    // The existing entry is not preferred and the
                    // current line has the preferred entry
                    DBOUT << lineNumber << " Replacing entry for " << geonameid << " (" << language << ") since new entry has preferrence.\n";
                    const std::string alternativeName = std::string(strvs.at(3));
                    nameInfo.first = alternativeName;
                }
                else
                {
                    DBOUT << lineNumber << " Didn't place " << geonameid << " (" << language << ") since there was already an entry.\n";
                }
            }
        }
    }
    std::cout << "Finished creating \"alternateData\"\n";
    return alternateData;
}

void printAdminDataMap(const std::unordered_map<Admin1Code, std::pair<std::string, GeoNameId>> &m)
{
    std::cout << "\n==========PRINT AdminData MAP==========\n";
    for (const auto &kvAdmin1Code : m)
    {
        std::cout << "Admin1Code: " << kvAdmin1Code.first << "\n";
        const auto &nameGeoId = kvAdmin1Code.second;
        std::cout << "\tLatinized: " << nameGeoId.first << "\n";
        std::cout << "\tGeoNameId: " << nameGeoId.second << "\n";
    }
    std::cout << "\n==========PRINT AdminData MAP==========\n";
}

std::unordered_map<Admin1Code, std::pair<std::string, GeoNameId>> parseAdminData(const std::string &admin1CodesASCIIPath)
{

    // Admin1Code -> { LatinizedName, GeoNameId }
    std::unordered_map<Admin1Code, std::pair<std::string, GeoNameId>> adminData = {};

    std::ifstream inputFile = std::ifstream(admin1CodesASCIIPath);
    std::size_t lineNumber = 0;
    for (std::string line; std::getline(inputFile, line);)
    {
        lineNumber++;
        if (lineNumber % 100 == 0)
        {
            std::cout << "Currently on line " << lineNumber << "\n";
        }
        const auto strvs = split<4>(line, '\t');

        const Admin1Code admin1code = std::string(strvs.at(0));
        if (auto it = adminData.find(admin1code); it == adminData.end())
        {
            const GeoNameId geonameid = charDigitsToInt(strvs.at(3));
            const std::string latinName = std::string(strvs.at(1));
            DBOUT << lineNumber << " Inserting Admin1Code \"" << admin1code << "\" with geonameid \"" << geonameid << "\"\n";
            adminData.insert({admin1code, {latinName, geonameid}});
        }
        else
        {
            DBOUT << lineNumber << " Skipping Admin1Code \"" << admin1code << "\" since it already exists.\n";
        }
    }
    std::cout << "Finished creating \"adminData\"\n";

    return adminData;
}

class LocalizedCountryNameManager
{
    std::string countryLocalizationsPath;
    std::unordered_map<ISOLanguage, std::map<ISOCountryCode, std::string>> localizedCountryNames = {};

public:
    LocalizedCountryNameManager(const std::string &countryLocalizationsPath) : countryLocalizationsPath{countryLocalizationsPath} {}

    std::optional<std::string> getLocalizedCountryName(const ISOLanguage &lang, const ISOCountryCode &code) noexcept
    {
        // Check if exists in map first
        if (const auto languageIter = localizedCountryNames.find(lang); languageIter != localizedCountryNames.end())
        {
            const auto &countryMap = languageIter->second;
            // The language exists in the map
            if (const auto countryMapIter = countryMap.find(code); countryMapIter != countryMap.end())
            {
                return countryMapIter->second;
            }
            else
            {
                DBOUT << "The language doesn't exist in the map, and could not be loaded in.\n";
                return std::nullopt;
            }
        }
        else
        {
            // The language doesn't exist in the map, read the file and add it
            std::string filepath = countryLocalizationsPath;
            filepath += lang;
            filepath += ".json";
            if (!std::filesystem::exists(filepath))
            {
                DBOUT << "The file: \"" << filepath << "\" does not exist.\n";
                return std::nullopt;
            }
            DBOUT << "The file \"" << filepath << "\" exists, reading it.\n";

            std::ifstream file = std::ifstream(filepath);
            nlohmann::json json;
            try
            {
                json = nlohmann::json::parse(file);
            }
            catch (std::exception e)
            {
                DBOUT << "Received an exception.\n\t" << e.what() << "\n";
                return std::nullopt;
            }
            if (!json.is_object())
            {
                DBOUT << "Expected a JSON object, but didn't get one.\n";
                return std::nullopt;
            }
            for (const auto &kv : json.items())
            {
                const auto &countryCode = kv.key();
                if (countryCode.size() != 2)
                {
                    DBOUT << "Received invalid country code: \"" << countryCode << "\" for " << lang << ".\n";
                    continue;
                }
                const auto &value = kv.value();
                if (!value.is_string())
                {
                    DBOUT << "Value associated with country code \"" << countryCode << "\" was not a string for " << lang << ".\n";
                    continue;
                }
                const std::string svalue = value.get<std::string>();
                if (auto iter = localizedCountryNames.find(lang); iter != localizedCountryNames.end())
                {
                    DBOUT << "The language \"" << lang << "\" already exists in map, adding new value.\n";
                    auto &countryLocalizationMap = iter->second;
                    countryLocalizationMap[countryCode] = svalue;
                }
                else
                {
                    DBOUT << "Adding language \"" << lang << "\" to map.\n";
                    localizedCountryNames.insert({lang, {{countryCode, svalue}}});
                }
            }
            DBOUT << "Finished adding language \"" << lang << "\" to the map.\n";
            if (const auto localizationIter = localizedCountryNames.find(lang); localizationIter != localizedCountryNames.end())
            {
                const auto &countryLocalizationMap = localizationIter->second;
                if (const auto codeIter = countryLocalizationMap.find(code); codeIter != countryLocalizationMap.end())
                {
                    return codeIter->second;
                }
                else
                {
                    DBOUT << "Parsed \"" << filepath << "\" and coult not find code: " << code << "\n";
                    return std::nullopt;
                }
            }
            else
            {
                DBOUT << "Parsed \"" << filepath << "\" and coult not find language: " << lang << "\n";
                return std::nullopt;
            }
        }
    }
};

// Creates a txt file with values seperated by '\t' and entries by '\n'
void parseCities(
    std::ostream &outputFile,
    const std::string &citiesPath,
    const std::string &alternateNamesPath,
    const std::set<ISOLanguage> &SELECTED_LANGUAGES,
    const std::set<ISOCountryCode> &SELECTED_COUNTRIES,
    const std::string &admin1CodesASCIIPath,
    const std::string &countryLocalizationsPath) noexcept
{
    const auto alternateNamesMap = parseAlternateNames(alternateNamesPath, SELECTED_LANGUAGES, SELECTED_COUNTRIES);
    const auto admin1Map = parseAdminData(admin1CodesASCIIPath);

    std::ifstream inputFile = std::ifstream(citiesPath);
    std::size_t lineNumber = 0;
    for (std::string line; std::getline(inputFile, line);)
    {
        lineNumber++;
        if (lineNumber % 1'000 == 0)
        {
            std::cout << "Currently on line " << lineNumber << "\n";
        }
        const auto strvs = split<18>(line, '\t');

        // Set the country code
        const ISOCountryCode countryCode = std::string(strvs.at(8));
        if (SELECTED_COUNTRIES.size() != 0)
        {
            if (SELECTED_COUNTRIES.find(countryCode) != SELECTED_COUNTRIES.end())
            {
                continue;
            }
        }
        if (countryCode.size() != 2)
        {
            DBOUT << lineNumber << " Received invalid country code, was not 2-characters. Received: \"" << countryCode << "\".\n";
            continue;
        }

        Record r = {};

        // Set the country code
        r.countryCode = countryCode;

        // Set the geonameid
        const GeoNameId geonameid = charDigitsToInt(strvs.at(0));
        r.gni = geonameid;

        // Set location/geographic data
        r.location.timezone = static_cast<IANATimezone>(std::string(strvs.at(17)));
        r.location.latitude = std::string(strvs.at(4));
        r.location.longitude = std::string(strvs.at(5));

        // Attempt to set admin1Name data for latinized and locale
        {
            const Admin1Code admin1Code = countryCode + "." + std::string(strvs.at(10));
            const auto admin1MapIter = admin1Map.find(admin1Code);
            if (admin1MapIter != admin1Map.end())
            {
                // Found administrative region
                const auto &p = admin1MapIter->second;
                // Set the latinized admin1Name
                r.latinizedName.admin1Name = p.first;
                // Search for admin1 locale names
                const GeoNameId admin1GNI = p.second;
                if (const auto localeAdmin1NameIter = alternateNamesMap.find(admin1GNI); localeAdmin1NameIter != alternateNamesMap.end())
                {
                    const auto &languageMap = localeAdmin1NameIter->second;
                    for (const auto &language : SELECTED_LANGUAGES)
                    {
                        // Loop through selected languages
                        if (const auto languageMapIter = languageMap.find(language); languageMapIter != languageMap.end())
                        {
                            // An alternate name exists for the selected language
                            const auto &localizedName = languageMapIter->second.first;
                            const auto localizedIter = r.localizedNames.find(language);
                            if (localizedIter != r.localizedNames.end())
                            {
                                // An entry already exists, add the new name
                                localizedIter->second.admin1Name = localizedName;
                            }
                            else
                            {
                                // An entry does not exist, create it
                                LocalizedNames ln = {};
                                ln.admin1Name = localizedName;
                                r.localizedNames.insert({language, ln});
                            }
                        }
                    }
                }
                DBOUT << lineNumber << " Finished setting admin1 names.\n";
            }
            else
            {
                // Can't find administrative region
                DBOUT << lineNumber << " Could not find admin1 code.\n";
            }
        }

        // Atempt to set cityName for latinized and locale
        {
            // Set the latinized city name
            r.latinizedName.cityName = std::string(strvs.at(1));
            const auto altNameIter = alternateNamesMap.find(geonameid);
            if (altNameIter != alternateNamesMap.end())
            {
                const auto &languageMap = altNameIter->second;
                for (const auto &language : SELECTED_LANGUAGES)
                {
                    // Loop through selected languages
                    if (const auto languageMapIter = languageMap.find(language); languageMapIter != languageMap.end())
                    {
                        // An alternate name exists for the selected language
                        const auto &localizedName = languageMapIter->second.first;
                        const auto localizedIter = r.localizedNames.find(language);
                        if (localizedIter != r.localizedNames.end())
                        {
                            // An entry already exists, add the new name
                            localizedIter->second.cityName = localizedName;
                        }
                        else
                        {
                            // An entry does not exist, create it
                            LocalizedNames ln = {};
                            ln.cityName = localizedName;
                            r.localizedNames.insert({language, ln});
                        }
                    }
                }
            }
            else
            {
                DBOUT << lineNumber << " Could not find geonameid " << lineNumber << " in alternate names.\n";
            }
        }

        // Attempt to set countryName for latinized and locale
        {
            auto lcm = LocalizedCountryNameManager(countryLocalizationsPath);
            // Default latinized is English
            if (const auto englishLatinized = lcm.getLocalizedCountryName("en", countryCode); englishLatinized.has_value())
            {
                r.latinizedName.countryName = englishLatinized.value();
            }
            else
            {
                DBOUT << lineNumber << " Could not find english latinized country name for \"" << countryCode << "\".\n";
            }
            // Find localized for each selected languages
            for (const auto &language : SELECTED_LANGUAGES)
            {
                // Loop through selected languages
                if (const auto localizedName = lcm.getLocalizedCountryName(language, countryCode); localizedName.has_value())
                {
                    if (auto localizedIter = r.localizedNames.find(language); localizedIter != r.localizedNames.end())
                    {
                        auto &localizedData = localizedIter->second;
                        localizedData.countryName = localizedName.value();
                    }
                    else
                    {
                        LocalizedNames ln = {};
                        ln.countryName = localizedName.value();
                        r.localizedNames.insert({language, ln});
                    }
                }
                else
                {
                    DBOUT << lineNumber << " Could not find localized country name for language \"" << language << "\" and country \"" << countryCode << "\".\n";
                }
            }
        }

        r.toStreamAsTxt(outputFile);
        outputFile << '\n';
    }
}

#include <argparse/argparse.hpp>

int main(int argc, char *argv[])
{
    argparse::ArgumentParser program = argparse::ArgumentParser("GeoNamesJSON", "0.1");

    program.add_argument("--output", "-o")
        .help("specify output file path.")
        .required();
    program.add_argument("--cities", "-ct")
        .help("specify path to the cities.txt file.")
        .required();
    program.add_argument("--input", "-i")
        .help("specify path to all required input files. View README.md for more information.")
        .required();

    program.add_argument("--languages", "--language", "-l")
        .help("select languages to include in output file. Provide a comma-seperated list of 639-1 codes.");
    program.add_argument("--countries", "--country", "-cu")
        .help("select countries to include in output file. Defaults to every country. Provide a comma-seperated list of ISO 3166-1 codes.");

    try
    {
        program.parse_args(argc, argv);
    }
    catch (const std::runtime_error &err)
    {
        std::cerr << err.what() << std::endl;
        std::cerr << program << '\n';
        return 1;
    }

    std::string citiesArgument = program.get<std::string>("--cities");
    DBOUT << "cities argument: " << std::quoted(citiesArgument) << '\n';
    if (!std::filesystem::exists(citiesArgument))
    {
        std::cerr << "--cities argument was not an existing file. Received: " << std::quoted(citiesArgument) << "\n";
        return 1;
    }

    std::string inputArgument = program.get<std::string>("--input");
    DBOUT << "input argument: " << std::quoted(inputArgument) << '\n';
    if (!std::filesystem::exists(inputArgument))
    {
        std::cerr << "--input argument could not be found in filesystem. Received: " << std::quoted(inputArgument) << "\n";
        return 1;
    }
    if (!std::filesystem::is_directory(inputArgument))
    {
        std::cerr << "--input argument was not a folder. View README.md for more information. Received:" << std::quoted(inputArgument) << "\n";
        return 1;
    }
    if (!inputArgument.ends_with('/') && !inputArgument.ends_with('\\'))
    {
        inputArgument += '/';
    }

    std::string outputArgument = program.get<std::string>("--output");
    DBOUT << "output argument: " << std::quoted(outputArgument) << '\n';
    std::ofstream outputFile = std::ofstream(outputArgument);
    if (!outputFile.good())
    {
        std::cerr << "--output argument was not good. Could not open stream. Received: " << outputArgument << "\n";
        return 1;
    }

    std::set<ISOLanguage> SELECTED_LANGUAGES = {};
    try
    {
        auto languagesArgument = program.get<std::string>("--languages");
        const auto splits = split(languagesArgument, ',');
        for (const auto sv : splits)
        {
            if (sv.size() != 2)
            {
                std::cerr << "Language provided is not an ISO 639-1 code, it has more than 2 characters.\n";
                return 1;
            }
            for (const auto c : sv)
            {
                if (!(('a' <= c) && (c <= 'z')))
                {
                    std::cerr << "Language provided is not an ISO 639-1 code, the characters are not lowercase.\n";
                    return 1;
                }
            }
            SELECTED_LANGUAGES.insert(std::string(sv));
            DBOUT << "Added language \"" << sv << "\" to selected set.\n";
        }
    }
    catch (const std::exception &err)
    {
        DBOUT << "No selected languages for localization. Only english latinization will be provided.\n";
        DBOUT << "Received: " << err.what() << "\n";
    }

    std::set<ISOCountryCode> SELECTED_COUNTRIES = {};
    try
    {
        auto countriesArgument = program.get<std::string>("--countries");
        const auto splits = split(countriesArgument, ',');
        for (const auto sv : splits)
        {
            if (sv.size() != 2)
            {
                std::cerr << "Country code provided is not an ISO 3166-1 Alpha-2 code, it has more than 2 characters.\n";
                return 1;
            }
            for (const auto c : sv)
            {
                if (!(('A' <= c) && (c <= 'Z')))
                {
                    std::cerr << "Country code provided is not an ISO ISO 3166-1 Alpha-2 code, the characters are not uppercase.\n";
                    return 1;
                }
            }
            SELECTED_COUNTRIES.insert(std::string(sv));
            DBOUT << "Added country \"" << sv << "\" to selected set.\n";
        }
    }
    catch (const std::exception &err)
    {
        DBOUT << "No selected countries, every country will be included.\n";
        DBOUT << "Received: " << err.what() << "\n";
    }

    const std::string inputLocalizedCountriesFolderPath = inputArgument + "localized-countries/";
    const std::string inputAlternateNamesPath = inputArgument + "alternateNames.txt";
    const std::string inputAdmin1CodesASCIIPath = inputArgument + "admin1CodesASCII.txt";

    parseCities(outputFile, citiesArgument, inputAlternateNamesPath, SELECTED_LANGUAGES, SELECTED_COUNTRIES, inputAdmin1CodesASCIIPath, inputLocalizedCountriesFolderPath);

    return 0;
}
