const addon = require('../build/Release/rehearse20-native');

interface IRehearse20Native
{
    greet(strName: string): string;
    detect(): number;
    protoring(): number;
    OutputInitAndStartStream(): number;
    DecodeDataIntoPlayback(data: Buffer): number;
};

class Rehearse20 {
    constructor(name: string) {
        this._addonInstance = new addon.Rehearse20(name)
    }

    greet (strName: string) {
        return this._addonInstance.greet(strName);
    }

    detect () {
        return this._addonInstance.detect();
    }

    protoring () {
        return this._addonInstance.protoring();
    }

    OutputInitAndStartStream () {
        return this._addonInstance.OutputInitAndStartStream();
    }

    DecodeDataIntoPlayback (data: Buffer) {
        return this._addonInstance.DecodeDataIntoPlayback(data);
    }    

    // private members
    private _addonInstance: IRehearse20Native;
}

export = Rehearse20;
