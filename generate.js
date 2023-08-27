const { execSync } = require("child_process");
const path = require("path");
const cwd = process.cwd();
const readline = require('node:readline');
const fs = require("node:fs");

// Here are the configuration variables, feel free to edit them to alter file generation

const generateIntermediate = true; // This should be true most of the time
const haveLatinized = true; // This should always be true
// A list of ISO 639-1 codes.
const selected_languages = ["en", "fr", "ja", "zh"];
// A list of ISO 3166-1 Alpha-2 codes
const selected_countries = [];

const filename = "en+fr+ja+zh+latinized-world";
const input_path = path.join(cwd, "./input/");
const cities_path = path.join(input_path, "cities15000.txt");
const immediate_file = path.join(cwd, `./intermediate/${filename}.txt`); // Change this to alter immediate file name and location
const output_file = path.join(cwd, `./output/${filename}.json`); // Change this to alter output file name and location
const exe_path = path.join(cwd, "./GeneratorCPP/x64/Release/GeneratorCPP.exe"); // This will have to change depending on platform
// END of configuration

const start = async () => {
    console.log("Starting generation of intermediate file.");
    const selected_languages_command = selected_languages.length !== 0 ? `--languages "${selected_languages.map(el => el.trim().toLowerCase()).join(",")}"` : "";
    const selected_countries_command = selected_countries.length !== 0 ? `--countries "${selected_countries.map(el => el.trim().toUpperCase()).join(",")}"` : "";
    const command = [
        `"${exe_path}"`,
        `--cities "${cities_path}"`,
        `--output "${immediate_file}"`,
        `--input "${input_path}"`,
        selected_languages_command,
        selected_countries_command
    ].join(" ").replace(/\\/g, '/');
    if (generateIntermediate) {
        execSync(command, { stdio: 'inherit' });
    }

    console.log("Generated intermediate file from GeneratorCPP.");
    console.log("Located at: ", immediate_file);
    console.log("Started generation of JSON file.");

    const bufferSize = 20;
    let buffer = null;
    const ws = fs.createWriteStream(output_file);

    let lineNumber = 0;
    const rli = readline.createInterface({
        input: fs.createReadStream(immediate_file)
    });
    ws.write("[");
    for await (const line of rli) {
        lineNumber++;
        const chunks = line.split('\t');
        if (lineNumber % 1000 === 0) {
            console.log("On line", lineNumber);
        }

        const countryCode = chunks.at(0).toUpperCase();
        const timezone = chunks.at(1);
        const latitude = Number(chunks.at(2));
        const longitude = Number(chunks.at(3));

        const latinizedArray = [];
        if (haveLatinized) {
            // the cityName (won't be undefined)
            const cityName = chunks.at(4);

            // the admin1Name (can be undefined), it is the province/state name
            const admin1Name = chunks.at(5);

            // the countryName (cen be undefined)
            const countryName = chunks.at(6);

            latinizedArray.push(cityName);
            latinizedArray.push(admin1Name);
            latinizedArray.push(countryName);
        }

        const localizationCount = Number(chunks.at(7));
        const localizedArray = [];
        let startIndex = 8;
        for (let i = 0; i < localizationCount; i++) {
            const language = chunks.at(startIndex).toLowerCase();
            const cityNameLocale = chunks.at(startIndex + 1);
            const admin1NameLocale = chunks.at(startIndex + 2);
            const countryNameLocale = chunks.at(startIndex + 3);

            localizedArray.push([language, cityNameLocale, admin1NameLocale, countryNameLocale]);
            startIndex += 4;
        }

        const str = JSON.stringify([countryCode, timezone, latinizedArray, localizedArray, latitude, longitude]);
        if (buffer == null) {
            ws.write(str);
            buffer = [];
        } else {
            if (buffer.length === bufferSize) {
                for (let j = 0; j < buffer.length; j++) {
                    ws.write(",");
                    ws.write(buffer.at(j));
                }
                ws.write(",");
                ws.write(str);
                buffer = [];
            } else {
                buffer.push(str);
            }
        }
    }
    if (buffer !== null) {
        for (let j = 0; j < buffer.length; j++) {
            ws.write(",");
            ws.write(buffer.at(j));
        }
    }
    ws.write("]");
    ws.close();
}
start();

