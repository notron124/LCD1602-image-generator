export class LCDRenderer {
    constructor(canvas, lcd, cols = 16, rows = 2) {
        this.canvas = canvas;
        this.ctx = canvas.getContext('2d');
        this.lcd = lcd;

        this.cols = cols;
        this.rows = rows;

        this.pixelSize = 4;
        this.charGap = 1;
        this.gridEnabled = true;

        this.colors = {
            bg: '#001100',
            pixel: '#33ff33',
            backlight: '#0a1a0a'
        };

    this.resize();
    }
    resize() {
        const width = this.cols * (5 * this.pixelSize) + 
                        (this.cols - 1) * this.charGap + 
                        2 * this.charGap;
        const height = this.rows * (8 * this.pixelSize) +
                        (this.rows - 1) * this.charGap +
                        2 * this.charGap;

        this.canvas.width = width;
        this.canvas.height = height;
    }

    setScale(scale) {
        this.pixelSize = scale;
        this.resize();
        this.render();
    }
    
    setColors(bg, pixel, backlight) {
        this.colors = { bg, pixel, backlight };
        this.render();
    }

    toggleGrid() {
        this.gridEnabled = !this.gridEnabled;
        this.render();
    }

    render() {
        if (!this.lcd || !this.lcd.getDisplayData) {
            console.warn('LCD not initialized');
            return;
        }
        
        const display = this.lcd.getDisplayData();
        const { bg, pixel, backlight } = this.colors;
        
        this.ctx.fillStyle = backlight;
        this.ctx.fillRect(0, 0, this.canvas.width, this.canvas.height);
        
        for (let row = 0; row < this.rows; row++) {
            for (let col = 0; col < this.cols; col++) {
                if (display[row] && display[row][col] && display[row][col].pixels) {
                    this.drawChar(display[row][col].pixels, col, row);
                }
            }
        }
        
        if (this.gridEnabled && this.pixelSize > 2) {
            this.drawGrid();
        }
    }

    drawChar(pixels, col, row) {
        if (!pixels || pixels.length < 8) {
            console.warn(`Invalid pixels data at col=${col}, row=${row}`);
            return;

        }
        const startX = this.charGap + col * (5 * this.pixelSize + this.charGap);
        const startY = this.charGap + row * (8 * this.pixelSize + this.charGap);

        for (let y = 0; y < 8; y++) {
            const byte = pixels[y] || 0;

            for (let x = 0; x < 5; x++) {
                const isOn = (byte >> (4 - x)) & 1;
            
                this.ctx.fillStyle = isOn ? this.colors.pixel : this.colors.bg;
                this.ctx.fillRect(
                    startX + x * this.pixelSize,
                    startY + y * this.pixelSize,
                    this.pixelSize,
                    this.pixelSize
                );
            }
        }
    }

    drawGrid() {
        this.ctx.strokeStyle = 'rgba(255, 255, 255, 0.08)';
        this.ctx.lineWidth = 0.5;

        for (let y = 0; y <= this.rows * 8; y++) {
            const yPos = this.charGap + y * this.pixelSize +
                        Math.floor(y / 8) * this.charGap;
            this.ctx.beginPath();
            this.ctx.moveTo(0, yPos);
            this.ctx.lineTo(this.canvas.width, yPos);
            this.ctx.stroke();
        }

        for (let x = 0; x <= this.cols * 5; x++) {
            const xPos = this.charGap + x * this.pixelSize +
                        Math.floor(x / 5) * this.charGap;
            this.ctx.beginPath();
            this.ctx.moveTo(xPos, 0);
            this.ctx.lineTo(xPos, this.canvas.height);
            this.ctx.stroke();
        }
    }

    exportPNG() {
        return this.canvas.toDataURL('image/png');
    }

    downloadPNG(filename = 'lcd1602.png') {
        const link = document.createElement('a');
        link.download = filename;
        link.href = this.exportPNG();
        link.click();
    }
}