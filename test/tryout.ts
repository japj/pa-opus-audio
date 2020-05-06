const PoaExperimental = require("../lib/binding.js").PoaExperimental;

const poa = new PoaExperimental("mr-yeoman");

poa.tryout();

setTimeout(function () {
    console.log('Blah blah blah blah extra-blah');
    poa.detect();
    process.exit();
}, 3000000);