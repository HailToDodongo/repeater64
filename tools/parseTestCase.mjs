import fs from 'fs';

const testCasePath = process.argv[2];

const outDir = process.argv[3];
const content = fs.readFileSync(testCasePath, 'utf-8');
const lines = content.split('\n').map(line => line.trim()).filter(line => line.length > 0);

let testName = '';

const buff = new Uint8Array(320*240*2);
let buffLen = 0;

const flushFile = () => {
  if(testName && buffLen > 0) {
    const outPath = `${outDir}/${testName}.test`;
    fs.writeFileSync(outPath, buff.slice(0, buffLen));
    console.log(`Wrote ${buffLen} bytes to ${outPath}`);
    buffLen = 0;
    testName = '';
  }
}

for(const line of lines)
{
  if(line.startsWith('[Debug]'))continue;

  if(line.startsWith('TEST=')) {
    flushFile();
    testName = line.substring(5).trim();
    buffLen = 0;
    continue;
  }

  if(testName) {
    for(let i=0; i<line.length; i+=2) {
      const byteStr = line.substring(i, i+2);
      buff[buffLen++] = parseInt(byteStr, 16);
    }
  }
}

flushFile();