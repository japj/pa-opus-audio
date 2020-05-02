const addon = require('../build/Release/pa-opus-audio');

//use this for debug builds with meaningfull stack trace symbols 
//const addon = require('../build/Debug/pa-opus-audio');

interface IRehearse20Native {
    greet(strName: string): string;
    detect(): number;
    protoring(): number;

    OutputInitAndStartStream(): number;
    DecodeDataIntoPlayback(data: Buffer): number;
    OutputStopStream(): number;

    InputInitAndStartStream(): number;
    InputStopStream(): number;

    SetEncodedFrameAvailableCallBack(cb: Function): number;
};


/**
 * 
 */
class PoaInput {
    constructor() {
        this._addonInstance = new addon.Rehearse20("PoaInput")
    }

    initStartRecord(): number {
        return this._addonInstance.InputInitAndStartStream();
    }

    stopRecord():number {
        return this._addonInstance.InputStopStream();
    }

    setEncodedFrameAvailableCallback(cb:Function): number {
        return this._addonInstance.SetEncodedFrameAvailableCallBack(cb);
    }

    // private members
    private _addonInstance: IRehearse20Native;
}

class PoaOutput {
    constructor() {
        this._addonInstance = new addon.Rehearse20("PoaOutput")
    }

    initStartPlayback(): number {
        return this._addonInstance.OutputInitAndStartStream();
    }

    stopPlayback(): number {
        return this._addonInstance.OutputStopStream();
    }

    decodeAndPlay(data:Buffer): number {
        return this._addonInstance.DecodeDataIntoPlayback(data);
    }

    // private members
    private _addonInstance: IRehearse20Native;
}

/**
 * This is the experimental part of the api
 */
class PoaExperimental {
    /**
     * 
     * @param name name of the user
     */
    constructor(name: string) {
        this._addonInstance = new addon.Rehearse20(name)
    }

    /**
     * prints some info for `strName`
     * @param strName 
     */
    greet(strName: string): string {
        return this._addonInstance.greet(strName);
    }

    /**
     * 
     */
    detect(): number {
        return this._addonInstance.detect();
    }

    /**
     * 
     */
    protoring():number {
        return this._addonInstance.protoring();
    }

    /**
     * 
     */
    OutputInitAndStartStream():number {
        return this._addonInstance.OutputInitAndStartStream();
    }

    /**
     * Playback audio by decoding previously encoded data
     * @param data Buffer to be decoded into playback
     */
    DecodeDataIntoPlayback(data: Buffer):number {
        return this._addonInstance.DecodeDataIntoPlayback(data);
    }

    /**
     * Start the input stream
     */
    InputInitAndStartStream():number {
        return this._addonInstance.InputInitAndStartStream();
    }

    /*EncodeRecordingIntoData() {
        return this._addonInstance.EncodeRecordingIntoData();
    }*/

    /**
     * Setup callback for receiving encoded data that was recorded
     * @param cb callback for receiving encoded data
     */
    SetEncodedFrameAvailableCallBack(cb: Function):number {
        return this._addonInstance.SetEncodedFrameAvailableCallBack(cb);
    }

    // private members
    private _addonInstance: IRehearse20Native;
}

//export = Rehearse20;
export { 
    PoaInput,
    PoaOutput,
    PoaExperimental
};
