import { DEFAULT_FONT } from "./fonts.js";

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
        

        console.log('LCD initialized');
    }
            
    get cgramData() {
        return [...this.cgram];
    }

    loadFont(fontData) {
        if (!fontData || typeof fontData !== 'object') {
            console.warn('Invalid font data');
            return;
        }
        this.cgrom = { ...fontData };
    }

    setCustomChar(index, data) {
        if (index < 0 || index > 7) {
            console.warn('CGRAM index must be 0-7, got:', index);
            return;
        }
        
        if (!Array.isArray(data) || data.length < 8) {
            console.warn('Custom char data must be array of 8 bytes');
            return;
        }

        const baseAddr = index * 8;
        for (let i = 0; i < 8; i++) {
            this.cgram[baseAddr + i] = (data[i] || 0) & 0x1F;
        }
    }

    getCharPixels(code) {
        if (code >= 0 && code <= 7) {
            const baseAddr = code * 8;
            return [...this.cgram.slice(baseAddr, baseAddr + 8)];
        }
        
        const char = String.fromCharCode(code);
        
        if (this.cgrom[char]) {
            return [...this.cgrom[char]];
        }
        
        return this.cgrom[' '] ? [...this.cgrom[' ']] : new Array(8).fill(0);
    }

    getDisplayData() {
        const display = [];
        
        for (let y = 0; y < this.rows; y++) {
            display[y] = [];
            for (let x = 0; x < this.cols; x++) {
                const code = this.ddram[y][x];
                display[y][x] = {
                    code: code,
                    char: code >= 32 ? String.fromCharCode(code) : `[${code}]`,
                    pixels: this.getCharPixels(code)
                };
            }
        }
        
        return display;
    }

    print() {
        console.table(
            this.ddram.map(row => 
                row.map(code => {
                    if (code >= 32 && code <= 126) {
                        return String.fromCharCode(code);
                    } else if (code >= 0 && code <= 7) {
                        return `[C${code}]`; // кастомный символ
                    }
                    return `[${code}]`;
                })
            )
        );
    }

    setCursor(x, y) {
        this.cursor.x = Math.max(0, Math.min(this.cols - 1, x));
        this.cursor.y = Math.max(0, Math.min(this.rows - 1, y));
    }

    write(value) {
        if (typeof value === 'string') {
            for (const char of value) {
                this.writeChar(char);
            }
        } else if (typeof value === 'number') {

            if (value < 0 || value > 255) {
                console.warn('Character code out of range (0-255):', value);
                return;
            }
            this.writeCharCode(value);
        } else {
            console.warn('Unsupported value type:', typeof value, value);
        }
    }

    writeChar(char) {
        if (typeof char !== 'string' || char.length !== 1) {
            console.warn('writeChar expects a single character, got:', char);
            return;
        }
        const code = char.charCodeAt(0);
        this.writeCharCode(code);
    }

    writeCharCode(code) {
        if (code === undefined || code === null) {
            console.warn('Invalid character code');
            return;
        }
        
        if (this.cursor.y >= this.rows) {
            console.warn('Cursor out of bounds');
            return;
        }
        
        this.ddram[this.cursor.y][this.cursor.x] = code;
        
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

    writeAt(x, y, value) {
        this.setCursor(x, y);
        this.write(value);
    }

    fillLine(y, value) {
        if (y < 0 || y >= this.rows) {
            console.warn('Invalid line index:', y);
            return;
        }
        
        const line = String(value).padEnd(this.cols, ' ').substring(0, this.cols);
        this.setCursor(0, y);
        this.write(line);
    }

    getLine(y) {
        if (y < 0 || y >= this.rows) {
            console.warn('Invalid line index:', y);
            return '';
        }
        
        return this.ddram[y]
            .map(code => {
                if (code >= 32) return String.fromCharCode(code);
                if (code >= 0 && code <= 7) return String.fromCharCode(code); // кастомный символ
                return ' ';
            })
            .join('')
            .trimEnd();
    }

    getText() {
        return [
            this.getLine(0),
            this.getLine(1)
        ];
    }

}