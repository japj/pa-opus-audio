/**
 * This script bootstraps vcpkg and uses it to install the required package port scripts
 */ 
const { execFileSync, spawn } = require('child_process');
const { existsSync } = require('fs');

var args = ['install'];

var options = {
    stdio: 'inherit', //feed all child process logging into parent process
    shell: true
};

function spawnBootstrap() {
    if (existsSync("./vcpkg/vcpkg") || existsSync(".\\vcpkg\\vcpkg.exe")) {
        spawnPortInstall();
        return;
    }
    else {
        const bootStrapScript = (process.platform === 'win32') ?
            "./vcpkg/bootstrap-vcpkg.bat" :
            "./vcpkg/bootstrap-vcpkg.sh";


        var bootstrapProcess = spawn(bootStrapScript, options);
        bootstrapProcess.on('close', function (code) {
            process.stdout.write('"bootstrap" finished with code ' + code + '\n');

            if (code != 0) {
                process.exit(code);
            }

            spawnPortInstall();
        });
    }
}

function spawnPortInstall() {
    const portInstallResponsefile = (process.platform == 'win32') ?
        "@vcpkg_x64-windows.txt" :
        (process.platform == 'darwin') ?
            "@vcpkg_x64-osx.txt" :
            "@vcpkg_x64-linux.txt"

    var portInstallProces = spawn("./vcpkg/vcpkg", ["install", portInstallResponsefile], options)

    portInstallProces.on('close', function (code) {
        process.stdout.write('"bootstrap" finished with code ' + code + '\n');
        process.exit(code);
    });
}

spawnBootstrap();