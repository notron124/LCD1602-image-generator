import { DEFAULT_FONT } from "./fonts";

export class LCD1602 {
    constructor() {
        this.cols = 16;
        this.rows = 2;

        this.ddram = [];
        this.cgram = new Array(64).fill(0);
        this.cgrom = { ...DEFAULT_FONT };

        this.cursor = {
            x: 0,
            y: 0
        };

        this.EMPTY = 32;


        for (let y = 0; y < this.rows; y++) {
            const row = [];

            for (let x = 0; x < this.cols; x++) {
                row.push(this.EMPTY);
            }

            this.ddram.push(row);
        }
        

        console.log('LCD initiallized');
    }
            
    loadFont(fontData) {
        this.cgrom = { ...fontData };
    }

    setCustomChar(index, data) {
        if (index < 0 || index > 7) {
            console.warn('CGRAM index must be 0-7');
            return;
        }

        const baseAddr = index * 8;
        for (let i = 0; i < 8; i++) {
            this.cgram[baseAddr + 1] = (data[i] || 0) & 0x1F;
        }
    }

    getCharPixels(code) {
        if (code >= 0 && code <= 7) {
            const baseAddr = code * 8;
            return this.cgram.slice(baseAddr, baseAddr + 8);
        } else {
            const char = String.fromCharCode(code);
            return this.cgrom[char] || this.cgrom[''] || new Array(8).fill(0);
        }
    }

    getDisplayData() {
        const display = [];
        for (let y = 0; y < this.rows; y++) {
            display[y];
            for (let x = 0; x < this.cols; x++) {
                const code = this.ddram[y][x];
                display[y][x] = {
                    code: code,
                    pixels: this.getCharPixels(code)
                };
            }
        }
        return display;
    }

    print() {
        console.table(this.ddram);
    }

    setCursor(x, y) {
        this.cursor.x = Math.max(0, Math.min(this.cols - 1, x));
        this.cursor.y = Math.max(0, Math.min(this.rows - 1, y));
    }

    write(value) {
        if (typeof value === 'string') {
            if (value.length > 1) {
                for (const char of value) {
                    this.writeChar(char);
                }
                return;
            }

            this.writeCharCode(value.charCodeAt);
            return;
        } else if (typeof value === 'number') {
            this.writeCode(value);
            return;
        }
        
        console.warn('Unsopported value: ', value);
    }

    writeChar(char) {
        const code = char.charCodeAt(0);
        this.writeCharCode(code);
    }

    writeCharCode(code) {

        const {x, y} = this.cursor;

        this.ddram[y][x] = code;

        this.cursor.x++;

        if (this.cursor.x >= this.cols) {
            this.cursor.x = 0;
            this.cursor.y++;
        

            if (this.cursor.y >= this.rows) {
                this.cursor.y = 0;
            }
        }
    }

    clear() {
        for (let y = 0; y < this.rows; y++) {
            for (let x = 0; x < this.cols; x++) {
                this.ddram[y][x] = this.EMPTY;
            }
        }
        this.setCursor(0, 0);
    }

}