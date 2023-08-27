export type ISOCountryCode = string; // An ISO 3166-1 Alpha-2 code
export type IANATimezone = string; // A timezone
export type ISOLanguage = string; // An ISO 639-1 code

// This is the localization associated with a locale
export interface Localization {
    cityName: string | undefined;
    admin1Name: string | undefined;
    countryName: string | undefined;
}

/**
 * ReverseGeoCodeResult is obtained by performing a query.
 */
export interface ReverseGeoCodeResult {
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

export class Geocode {
    /**
     * Init() creates a Geocode object based on the provided localization.
     * @param array The result of JSON.parse() a localization file.
     * @param maxDistance The maximum distance at which to search for the nearest city. (default: 100km)
     */
    static Init(array: any, maxDistance: number = 100): Geocode;
    query(latitude: number, longitude: number): ReverseGeoCodeResult|Error;
}