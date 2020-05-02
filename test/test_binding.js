/**
 * This is the "smoketest" of the binary api, it is intended to run on the buildserver to verify 
 * correct loading of all the binaries.
 */

const PoaExperimental = require("../lib/binding.js").PoaExperimental;
const assert = require("assert");

assert(PoaExperimental, "The expected module is undefined");

function testBasic()
{
    const instance = new PoaExperimental("mr-yeoman");
    assert(instance.greet, "The expected method is not defined");
    assert.strictEqual(instance.greet("kermit"), "mr-yeoman", "Unexpected value returned");
    assert.strictEqual(instance.detect(), 0, "Unexpected value returned");
}

function testInvalidParams()
{
    const instance = new PoaExperimental();
}

assert.doesNotThrow(testBasic, undefined, "testBasic threw an expection");
assert.throws(testInvalidParams, undefined, "testInvalidParams didn't throw");

console.log("Tests passed- everything looks OK!");