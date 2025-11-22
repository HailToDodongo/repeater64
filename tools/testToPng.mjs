import fs from 'fs';
import {PNG} from 'pngjs';

const fileIn = process.argv[2];
const fileOut = process.argv[3];

// In: buffer of RGBA16 values (5551)
// Out: PNG image

const WIDTH = 320-16-15;
const HEIGHT = 240-48-48;

const inData = fs.readFileSync(fileIn);

const png = new PNG({width: WIDTH, height: HEIGHT});

let inIdx = 0;
for(let y=0; y<HEIGHT; ++y) {
  for(let x=0; x<WIDTH; ++x) {
    const pixel = (inData[inIdx] << 8) | inData[inIdx+1];
    inIdx += 2;

    const r = ((pixel >> 11) & 0x1F) << 3;
    const g = ((pixel >> 6) & 0x1F) << 3;
    const b = ((pixel >> 1) & 0x1F) << 3;
    const a = (pixel & 0x01) ? 255 : 0;

    const idx = (y * WIDTH + x) * 4;
    png.data[idx] = r;
    png.data[idx+1] = g;
    png.data[idx+2] = b;
    png.data[idx+3] = a;
  }
}

png.pack().pipe(fs.createWriteStream(fileOut)).on('finish', () => {
  console.log(`Wrote PNG to ${fileOut}`);
});
