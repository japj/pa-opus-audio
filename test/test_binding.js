const Rehearse20 = require("../dist/binding.js");
const assert = require("assert");

assert(Rehearse20, "The expected module is undefined");

function testBasic()
{
    const instance = new Rehearse20("mr-yeoman");
    assert(instance.greet, "The expected method is not defined");
    assert.strictEqual(instance.greet("kermit"), "mr-yeoman", "Unexpected value returned");
}

function testInvalidParams()
{
    const instance = new Rehearse20();
}

assert.doesNotThrow(testBasic, undefined, "testBasic threw an expection");
assert.throws(testInvalidParams, undefined, "testInvalidParams didn't throw");

console.log("Tests passed- everything looks OK!");