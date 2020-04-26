const addon = require('../build/Release/rehearse20-native');

interface IRehearse20Native
{
    greet(strName: string): string;
};

class Rehearse20 {
    constructor(name: string) {
        this._addonInstance = new addon.Rehearse20(name)
    }

    greet (strName: string) {
        return this._addonInstance.greet(strName);
    }

    // private members
    private _addonInstance: IRehearse20Native;
}

export = Rehearse20;
