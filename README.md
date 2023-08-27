# LocalizedGeocodeJS

Perform reverse geocoding offline and receive localized information.

Information provided:
- Country code (ISO 3166-1 Alpha-2)
- Timezone (ISO 639-1 code)
- Latinization (An English latinization that may include accents)
- Available locales (Based on input file)

The location name information is:
- City name (or the name of a neighborhood)
- Admin1 region name (the name of state/province/territory/etc.)
- Country name

The query function can be inacurate since it find the nearest data point. 
The city name returned may be the name of a neighborhood.

# Data Types

```typescript
type ISOCountryCode = string; // An ISO 3166-1 Alpha-2 code
type IANATimezone = string; // A timezone
type ISOLanguage = string; // An ISO 639-1 code

// This is the localization associated with a locale
interface Localization {
    cityName: string | undefined;
    admin1Name: string | undefined;
    countryName: string | undefined;
}

/**
 * ReverseGeoCodeResult is obtained by performing a query.
 */
interface ReverseGeoCodeResult {
    countryCode: ISOCountryCode;
    timezone: IANATimezone;
    /**
     * This is the English latinization, it may still contain accents.
     * It may not be ASCII.
     */
    latinized: {
        cityName: string;
        admin1Name: string | undefined;
        countryName: string | undefined;
    };
    locales: Record<ISOLanguage, Localization>;
}
```

The `latinized` field contains an English latinization of the place name. The latinization may include accents.

The `locales` field is an object mapping a language to its localization. The localization must be present in file.

# How to Use

If you want a way to reverse geocode and get english names, you will only need the file `./output/only-latinized-english-world.json` and `query.js`.

If you want to generate localization for any language (or pair thereof), you will need to generate them using `generate.js`. On a slow computor, generating should not take more than a few minutes and not allocate more than 100 MB (unless using the empty default language).

## generate.js

Before using `generate.js`, ensure that you have the supported file structure in the input folder.

```
.
├── ...
├──input
│  ├─── localized-countries
│  │    ├───────────────── af.json       # There are over 600 of these localization files
│  │   ...                 ...
│  │   ...                 ...
│  │   ...                 ...
│  │    └───────────────── zu.json
│  ├─── admin1CodesASCII.txt             # The name of admin1 divisions
│  ├─── alternateNames.txt               # The altername name data
│  └─── cities[n].txt                    # The location/name of cities
└──...
```

Note: The cities[n].txt where [n] is a number. The cities file comes in 4 varieties: `cities500.txt`, `cities1000.txt`, `cities5000.txt`, and `cities15000.txt`. Larger number means less cities, less acuracy, but far less file size.

Note: The files directly in the input folder can be be found on [GeoNames.org](https://download.geonames.org/export/dump/).

Notes: The folder `localized-countries` correspondes to the `data` folder in the project [localized-countries](https://github.com/marcbachmann/localized-countries).

Once you have the files and structure, you can change the variables at the top of the `generate.js` to customize your output files.

The output files in a json format. However to save space, arrays are used instead of objects with keys. 
The files are typically about 3000 KB for single localiztion and latinization. Only latinization is 2100 KB.
When gzipped, the same file can be about 500 KB. 


## Use in your projects

1. Read in the desired localization file.
2. JSON.parse it to get an array
3. Pass array to Geocode.Init
4. Run reverse geocodes via geocode.query

Minimal example:

```javascript
const fs = require("node:fs");
const { Geocode } = require("./query");
const json = JSON.parse(fs.readFileSync("./output/only-latinized-world.json"));
const g = Geocode.Init(json);
const q = g.query(40.710181, -74.007358);

console.log(q);
```

# Data sources

The files `cities15000.txt`, `alternateNames.txt`, and `admin1CodesASCII.txt` all come from [GeoNames.org](https://download.geonames.org/export/dump/). None have been edited. [GeoNames Homepage](https://www.geonames.org/).

The files located in the folder `localized-countries` all come from the [localized-countries](https://github.com/marcbachmann/localized-countries) project. None have been edited.

# Dependencies

## GeneratorCPP (C++)

The C++ Visual Studio project `GeneratorCPP` depends on [nlohmann/json](https://github.com/nlohmann/json) and [argparse](https://github.com/p-ranav/argparse).

## generate.js/query.js

The script `generate.js` has no dependencies except for GeneratorCPP  which is included in the project.

The node scripts should work on all current versions of node, however it was only tested on v18.17.0 and Windows. It will not work in its current state on other platforms.

The file `query.js` depends on [kd-tree-javascript](https://www.npmjs.com/package/kd-tree-javascript).

# Thanks

Thanks to [GeoNames.org](https://www.geonames.org/) for making geospacial data freely available.

This project was also inspired by [local-reverse-geocoder](https://github.com/tomayac/local-reverse-geocoder).
