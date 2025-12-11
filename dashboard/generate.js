import fs from 'fs-extra';
import path from 'path';
import zlib from 'zlib';

const DIST = path.resolve('./dist/index.html');
const OUT = path.resolve('../src/assets/dashboard.c');

const html = await fs.readFile(DIST, 'utf8');

const gzipped = zlib.gzipSync(html, { level: 9 });

const varName = 'index_html_gz';
const cArray = gzipped
    .reduce((acc, byte, i) => {
        const s = byte.toString().padStart(4, ' ') + ((i + 1) % 15 === 0 ? ',\n    ' : ',');
        return acc + s;
    }, '    ');


const cCode = `#include <stdint.h>
#include <stddef.h>

const uint8_t ${varName}[] = {\n${cArray}\n};
const size_t ${varName}_len = ${gzipped.length};`;

// Ensure output directory exists
await fs.ensureDir(path.dirname(OUT));
await fs.writeFile(OUT, cCode);

console.log(`Generated ${OUT}, size: ${gzipped.length} bytes`);
