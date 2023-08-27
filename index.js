const { kdTree } = require("kd-tree-javascript");

class Geocode { 
    static #isInternalConstructing = false;
    static #distance(x, y) {
        // Code taken from:
        // https://github.com/tomayac/local-reverse-geocoder/blob/main/index.js
        // https://www.movable-type.co.uk/scripts/latlong.html
        const RAD_CONVERT = Math.PI / 180;
        const RADIUS = 6371;
        const lat1 = x.latitude;
        const lon1 = x.longitude;
        const lat2 = y.latitude;
        const lon2 = y.longitude;
        const φ1 = (lat1) * RAD_CONVERT;
        const φ2 = (lat2) * RAD_CONVERT;
        const Δφ = (lat2 - lat1) * RAD_CONVERT;
        const Δλ = (lon2 - lon1) * RAD_CONVERT;
        const a = Math.sin(Δφ / 2) * Math.sin(Δφ / 2) + Math.cos(φ1) * Math.cos(φ2) * Math.sin(Δλ / 2) * Math.sin(Δλ / 2);
        const c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a));
        return RADIUS * c;
    }
    static #emptyToUndefined(str) {
        return str !== "" ? str : undefined;
    }
    static #parseData(arr) {
        const countryCode = arr[0];
        const timezone = arr[1];
        const latinizedArray = arr[2];
        const localizedArray = arr[3];

        const localizedObject = {};
        for (const localization of localizedArray) {
            const language = localization.at(0);
            const cityName = Geocode.#emptyToUndefined(localization.at(1));
            const admin1Name = Geocode.#emptyToUndefined(localization.at(2));
            const countryName = Geocode.#emptyToUndefined(localization.at(3));

            localizedObject[language] = {
                cityName,
                admin1Name,
                countryName
            };
        }
        return {
            countryCode,
            timezone,
            latinized: {
                cityName: Geocode.#emptyToUndefined(latinizedArray.at(0)),
                admin1Name: Geocode.#emptyToUndefined(latinizedArray.at(1)),
                countryName: Geocode.#emptyToUndefined(latinizedArray.at(2))
            },
            locales: localizedObject,
        };
    }
    #maxDistance;
    #tree;

    static Init(array, maxDistance = 100) {
        Geocode.#isInternalConstructing = true;
        const instance = new Geocode(array, maxDistance);
        return instance;
    }
    constructor(array, maxDistance = 100) {
        if (!Geocode.#isInternalConstructing) {
            throw new TypeError("Geocode is not constructable, please use Geocode.Init instead.");
        }
        Geocode.#isInternalConstructing = false;
        if (arguments.length !== 2) {
            throw new TypeError(`Geocode requires 2 argument, but received ${arguments.length} arguments.`);
        }
        if (!Array.isArray(array)) {
            throw new TypeError("Geocode requires 1 argument, it must be an array.");
        }
        const points = [];
        for (const element of array) {
            if (!Array.isArray(element)) {
                throw new Error("Error parsing array, all first level elements must be an array.");
            }
            const longitude = element.pop();
            const latitude = element.pop();
            const point = {
                latitude: latitude,
                longitude: longitude,
                data: element
            }
            points.push(point);
        }
        this.#maxDistance = maxDistance;
        this.#tree = new kdTree(points, Geocode.#distance, ["latitude", "longitude"]);
    }
    query(latitude, longitude) {
        const nearest = this.#tree.nearest({ 
            latitude: latitude, longitude: longitude 
        }, 1, this.#maxDistance);
        if (nearest.length < 1) {
            return new Error("Could not find city within maximum search distance.");
        } else {
            const data = nearest.at(0)?.[0]?.data;
            if (data === undefined) {
                return new Error("Error whilst retriving data.");
            }
            return Geocode.#parseData(data);
        }
    }
    
}
module.exports = {
    Geocode
}