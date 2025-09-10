const fs = require('fs');
var path = require('path');
const { exec } = require('child_process');


function fromDir(startPath, filter) {
    if (!fs.existsSync(startPath)) {
        console.log("no dir ", startPath);
        return;
    }

    var files = fs.readdirSync(startPath);
    for (var i = 0; i < files.length; i++) {
        var filename = path.join(startPath, files[i]);
        var stat = fs.lstatSync(filename);
        if (stat.isDirectory()) {
            fromDir(filename, filter); //recurse
        } else if (filename.indexOf(filter) >= 0) {
            console.log('-- found: ', filename);
        }
    }
}


//Traverse directories recursively
function walkDir(dir, callback) {
    fs.readdirSync(dir).forEach(f => {
        let dirPath = path.join(dir, f);
        let isDirectory = fs.statSync(dirPath).isDirectory();
        isDirectory ?
            walkDir(dirPath, callback) : callback(path.join(dir, f));
    });
};


var sourceFileMap = new Map();
var targetFileMap = new Map();
var exportMode = "OVERRIDE_MODE";
const SPLIT_STR = "# import PO file from volcengine";


function genFileMaps(){

    const args = process.argv.slice(2);
    console.log(args);

    var sourcePath = args[0];
    var targetPath = args[1];

    walkDir(sourcePath, function (filePath) {
        if (path.extname(filePath) === '.po') {
            //Get the file name of the parent directory. According to the law,
            // the parent directory of the .po file is the text of the language category
            let parentDirPath = path.dirname(filePath);
            var parentDirName = path.basename(parentDirPath);
            sourceFileMap.set(parentDirName, filePath);
        }
    });


    walkDir(targetPath, function (filePath) {
        if (path.extname(filePath) === '.po') {
            //Get the file name of the parent directory. According to the law,
            // the parent directory of the .po file is the text of the language category
            let parentDirPath = path.dirname(filePath);
            var parentDirName = path.basename(parentDirPath);
            targetFileMap.set(parentDirName, filePath);
        }
    });
}

function doExport() {
    //pass aprameters
    const args = process.argv.slice(2);
    console.log(args);

    var sourcePath = args[0];
    var targetPath = args[1];

    walkDir(sourcePath, function (filePath) {
        if (path.extname(filePath) === '.po') {
            //Get the file name of the parent directory. According to the law,
            // the parent directory of the .po file is the text of the language category
            let parentDirPath = path.dirname(filePath);
            var parentDirName = path.basename(parentDirPath);
            sourceFileMap.set(parentDirName, filePath);
        }
    });


    walkDir(targetPath, function (filePath) {
        if (path.extname(filePath) === '.po') {
            //Get the file name of the parent directory. According to the law,
            // the parent directory of the .po file is the text of the language category
            let parentDirPath = path.dirname(filePath);
            var parentDirName = path.basename(parentDirPath);
            targetFileMap.set(parentDirName, filePath);
        }
    });

    for (let [key, value] of sourceFileMap.entries()) {
        if (targetFileMap.has(key)) {
            try {
                console.log("current handle file : ", targetFileMap.get(key));
                var fd = fs.openSync(targetFileMap.get(key), 'r+');
                var fileContent = fs.readFileSync(targetFileMap.get(key), 'utf8');
                var importFileContent = fs.readFileSync(value, 'utf8');
                // 
                const regex = /msgid "([^"]+)"/;
                const match = regex.exec(importFileContent);
                if (match) {
                    console.log(`The first non-empty msgid is at position: ${match.index}`);
                    importFileContent = importFileContent.substring(match.index - 6);
                } else {
                    console.log('No non-empty msgid found, stop importing this po files');
                    return;
                }

                var iIndx = fileContent.indexOf(SPLIT_STR);
                if (iIndx != -1) {
                    if (exportMode == "OVERRIDE_MODE") {
                        var writeContent = SPLIT_STR;
                        writeContent +=  importFileContent;
                        //clear the content after iIndx
                        //fs.ftruncateSync(fd, iIndx);
                        let newContent = fileContent.substring(0,iIndx);
                        newContent +=  writeContent;   
                        fs.writeFile(targetFileMap.get(key), newContent, (err) => {
                            if (err) {
                                console.log(`File: '+ targetFileMap.get(key) + 'override written failed. error msg is : ${err}`);
                            } else {
                                console.log('File: '+ targetFileMap.get(key) + 'override written successfully.');
                            }
                        });
                        // fs.writeFileSync(fd,newContent);    
                        // fs.appendFileSync(targetFileMap.get(key), writeContent);
                        // fs.closeSync(fd);
                        // console.log('File written successfully.');            
                    }
                    else {
                        fs.appendFileSync(targetFileMap.get(key), importFileContent);
                        console.log('File: '+ targetFileMap.get(key) + 'written successfully.');
                        fs.closeSync(fd);
                    }
                }
                else {
                    //write empty line
                    var newContent = "";
                    newContent += "\r\n";
                    newContent += SPLIT_STR;
                    newContent += importFileContent;
                    //fs.writeSync(fd, newContent, fileContent.length);
                    fs.appendFileSync(targetFileMap.get(key), newContent);
                    console.log(`File ${targetFileMap.get(key)} written successfully.`);
                    fs.closeSync(fd);
                }
            }
            catch (err) {
                console.error('Error:', err);
            }
        }
    }
}

function doBuildMoFiles() {
    genFileMaps();
    //start cmdline  to build po files to mo files
    for (let [key, value] of targetFileMap.entries()) {
        //only need to generate the mo file of the exported po file    
        if (!sourceFileMap.has(key)) {
            continue;
        }

        let commadLine = "msgfmt -o ";
        let filenameWithExtention = path.parse(path.basename(value)).base;
        // let filename = path.parse(path.basename(value)).name;
        // //should split the [_language] suffix in the filename
        // let iSuffixIndex = filename.lastIndexOf("_");
        // filename = filename.substring(0, iSuffixIndex);
        // filename += ".mo";
        // filename = path.dirname(value) + "\\" + filename;
        
        //just set mo file name to AnkerMake_alpha.mo
        let filename  = path.dirname(value) + "\\" + "\"AnkerMake Studio.mo\"";
        commadLine += filename;
        commadLine += " ";
        commadLine += "\"";
        commadLine += value;
        commadLine += "\"";

        console.log("excute cmd : ", commadLine);

        exec(commadLine, (error, stdout, stderr) => {
            if (error) {
                console.error(`Build ${filename}.mo file error, error msg: ${error}`);
                return;
            }
        });
        console.log("success to generate mo file: ", filename); 
    }
}


const readline = require('readline');
const rl = readline.createInterface({
    input: process.stdin,
    output: process.stdout
});


console.log(`******************************************
*1:Append import po and build Mo                                *
*2:Append import Po files                                    *
*3:Override importpo and build Mo                                *
*4:Override importpo Po files                                    *
*5:Build Mo files                                        *
*Please enter the specific working mode: just enter the specific number like 1,2,3... ect *
*********************************************************`);

rl.question(``, (answer) => {
    switch (answer) {
        case '1':
            {
                exportMode = "APPEND_MODE";
                console.log('Append import po and build Mo  ');
                console.log("start export------------->");
                doExport();
                console.log("exit export------------->");
                console.log("start build Po files------------->");
                doBuildMoFiles();
                console.log("exit build Po files------------->");
                break;
            }

        case '2':
            {
                exportMode = "APPEND_MODE";
                console.log('Append import po');
                console.log("start export------------->");
                doExport();
                console.log("exit export------------->");
                break;
            }

        case '3':
            {
                console.log('Override import po and build Mo ');
                console.log("start export------------->");
                doExport();
                console.log("exit export------------->");
                console.log("start build Po files------------->");
                doBuildMoFiles();
                console.log("exit build Po files------------->");
                break;
            }

        case '4':
            {
                console.log('Override import po ');
                console.log("start export------------->");
                doExport();
                console.log("exit export------------->");
                break;
            }

        case '5':
            {
                console.log('build  Mo files');
                console.log("start build Po files------------->");
                doBuildMoFiles();
                console.log("exit build Po files------------->");
                break;
            }
        default:
            console.log('invalid input');
    }
    rl.close();
});


//satrt import
// console.log("start export------------->");
// doExport();
// console.log("exit export------------->");
// console.log("start build Po files------------->");
// doBuildMoFiles();
// console.log("exit build Po files------------->");