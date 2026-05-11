// main.js
import { LCD1602 } from './lcd1602.js';
import { LCDRenderer } from './renderer.js';

// ==================== ИНИЦИАЛИЗАЦИЯ ====================

// Создаём экземпляры
const lcd = new LCD1602();
const canvas = document.getElementById('lcd');
const renderer = new LCDRenderer(canvas, lcd);

// Ссылки на DOM-элементы
const elements = {
    line1: document.getElementById('line1'),
    line2: document.getElementById('line2'),
    colorSelect: document.getElementById('colors'),
    scaleSelect: document.getElementById('scale'),
    showGrid: document.getElementById('showGrid'),
    saveImage: document.getElementById('saveImageBtn'),
    saveProject: document.getElementById('saveProjectBtn'),
    loadProject: document.getElementById('loadProjectBtn')
};

// Проверяем, что все элементы найдены
for (const [name, element] of Object.entries(elements)) {
    if (!element) {
        console.error(`Element not found: ${name}`);
    }
}

// ==================== КОНФИГУРАЦИЯ ====================

// Цветовые схемы
const colorSchemes = {
    green: {
        name: 'Зелёный (оригинальный)',
        bg: '#001100',
        pixel: '#33ff33',
        backlight: '#0a1a0a'
    },
    black: {
        name: 'Чёрно-белый',
        bg: '#000000',
        pixel: '#ffffff',
        backlight: '#1a1a1a'
    },
    blue: {
        name: 'Синий',
        bg: '#000022',
        pixel: '#3366ff',
        backlight: '#0a0a1a'
    }
};

// Соответствие значений селекта и числового масштаба
const scaleMap = {
    'scale1': 1,
    'scale2': 2,
    'scale3': 3,
    'scale4': 4,
    'scale8': 8
};

// Настройки по умолчанию
const defaults = {
    text: ['Hello, World!', 'LCD1602 Emulator'],
    colorScheme: 'green',
    scale: 'scale2',
    showGrid: true
};

// ==================== ФУНКЦИИ ОБНОВЛЕНИЯ ====================

/**
 * Обновляет текст на LCD из полей ввода
 */
function updateLCDText() {
    // Сбрасываем дисплей
    lcd.clear();
    
    // Получаем текст из полей
    const line1Text = elements.line1.value || '';
    const line2Text = elements.line2.value || '';
    
    // Пишем первую строку
    lcd.setCursor(0, 0);
    lcd.write(line1Text);
    
    // Пишем вторую строку
    lcd.setCursor(0, 1);
    lcd.write(line2Text);
}

/**
 * Полное обновление дисплея (текст + рендер)
 */
function refreshDisplay() {
    updateLCDText();
    renderer.render();
}

// ==================== ОБРАБОТЧИКИ СОБЫТИЙ ====================

// Ввод текста - живое обновление
elements.line1.addEventListener('input', refreshDisplay);
elements.line2.addEventListener('input', refreshDisplay);

// Дополнительно: перенос фокуса по Enter
elements.line1.addEventListener('keydown', (e) => {
    if (e.key === 'Enter') {
        e.preventDefault();
        elements.line2.focus();
    }
});

elements.line2.addEventListener('keydown', (e) => {
    if (e.key === 'Enter') {
        e.preventDefault();
        elements.line1.focus();
    }
});

// Изменение цветовой схемы
elements.colorSelect.addEventListener('change', (e) => {
    const scheme = colorSchemes[e.target.value];
    if (scheme) {
        renderer.setColors(scheme.bg, scheme.pixel, scheme.backlight);
        console.log(`Color scheme changed to: ${scheme.name}`);
    }
});

// Изменение масштаба
elements.scaleSelect.addEventListener('change', (e) => {
    const scale = scaleMap[e.target.value];
    if (scale) {
        renderer.setScale(scale);
        console.log(`Scale changed to: ${scale}x`);
    }
});

// Переключение сетки
elements.showGrid.addEventListener('change', (e) => {
    renderer.gridEnabled = e.target.checked;
    renderer.render();
    console.log(`Grid ${e.target.checked ? 'enabled' : 'disabled'}`);
});

// ==================== СОХРАНЕНИЕ И ЗАГРУЗКА ====================

// Сохранение изображения
elements.saveImage.addEventListener('click', () => {
    const timestamp = new Date().toISOString().replace(/[:.]/g, '-').slice(0, 19);
    const filename = `lcd1602-${timestamp}.png`;
    
    renderer.downloadPNG(filename);
    console.log(`Image saved: ${filename}`);
});

// Сохранение проекта
elements.saveProject.addEventListener('click', () => {
    // Получаем текущие CGRAM данные (кастомные символы)
    const cgramData = Array.from(lcd.cgram); // предполагаем что добавили геттер
    
    const project = {
        version: '1.1',
        timestamp: new Date().toISOString(),
        text: [
            elements.line1.value,
            elements.line2.value
        ],
        colors: {
            bg: renderer.colors.bg,
            pixel: renderer.colors.pixel,
            backlight: renderer.colors.backlight
        },
        scale: renderer.pixelSize,
        gridEnabled: renderer.gridEnabled,
        customChars: cgramData, // сохраняем кастомные символы
        colorScheme: elements.colorSelect.value // для точного восстановления
    };
    
    // Создаём и скачиваем файл
    const jsonStr = JSON.stringify(project, null, 2);
    const blob = new Blob([jsonStr], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    
    const timestamp = new Date().toISOString().replace(/[:.]/g, '-').slice(0, 19);
    const link = document.createElement('a');
    link.download = `lcd1602-project-${timestamp}.json`;
    link.href = url;
    link.click();
    
    // Очищаем URL
    URL.revokeObjectURL(url);
    console.log('Project saved successfully');
});

// Загрузка проекта
elements.loadProject.addEventListener('click', () => {
    const input = document.createElement('input');
    input.type = 'file';
    input.accept = '.json,application/json';
    
    input.addEventListener('change', (e) => {
        const file = e.target.files[0];
        if (!file) return;
        
        const reader = new FileReader();
        
        reader.addEventListener('load', (event) => {
            try {
                const project = JSON.parse(event.target.result);
                loadProject(project);
            } catch (error) {
                alert(`Ошибка при чтении проекта:\n${error.message}`);
                console.error('Project load error:', error);
            }
        });
        
        reader.addEventListener('error', () => {
            alert('Ошибка при чтении файла');
            console.error('File read error');
        });
        
        reader.readAsText(file);
    });
    
    input.click();
});

/**
 * Восстанавливает состояние из объекта проекта
 */
function loadProject(project) {
    // Восстанавливаем текст
    if (project.text && Array.isArray(project.text)) {
        elements.line1.value = project.text[0] || '';
        elements.line2.value = project.text[1] || '';
    }
    
    // Восстанавливаем цвета
    if (project.colors) {
        renderer.setColors(
            project.colors.bg || defaults.colors.bg,
            project.colors.pixel || defaults.colors.pixel,
            project.colors.backlight || defaults.colors.backlight
        );
    }
    
    // Восстанавливаем выбор цветовой схемы
    if (project.colorScheme && colorSchemes[project.colorScheme]) {
        elements.colorSelect.value = project.colorScheme;
    }
    
    // Восстанавливаем масштаб
    if (project.scale) {
        renderer.setScale(project.scale);
        
        // Находим соответствующее значение в селекте
        const scaleEntry = Object.entries(scaleMap).find(([, v]) => v === project.scale);
        if (scaleEntry) {
            elements.scaleSelect.value = scaleEntry[0];
        }
    }
    
    // Восстанавливаем сетку
    if (project.gridEnabled !== undefined) {
        renderer.gridEnabled = project.gridEnabled;
        elements.showGrid.checked = project.gridEnabled;
    }
    
    // Восстанавливаем кастомные символы, если они есть
    if (project.customChars && Array.isArray(project.customChars) && project.customChars.length === 64) {
        for (let i = 0; i < 64; i++) {
            lcd.cgram[i] = project.customChars[i] & 0x1F;
        }
    }
    
    // Обновляем отображение
    refreshDisplay();
    console.log('Project loaded successfully');
}

// ==================== ДОПОЛНИТЕЛЬНЫЕ ФУНКЦИИ ====================

/**
 * Проверяет, все ли модули загружены корректно
 */
function checkDependencies() {
    const checks = [
        { name: 'LCD1602', object: lcd },
        { name: 'LCDRenderer', object: renderer },
        { name: 'Canvas', object: canvas },
        { name: 'Canvas Context', object: canvas.getContext('2d') }
    ];
    
    let allOk = true;
    checks.forEach(check => {
        if (!check.object) {
            console.error(`❌ ${check.name} not initialized`);
            allOk = false;
        } else {
            console.log(`✅ ${check.name} ready`);
        }
    });
    
    return allOk;
}

/**
 * Сброс к настройкам по умолчанию
 */
function resetToDefaults() {
    elements.line1.value = defaults.text[0];
    elements.line2.value = defaults.text[1];
    elements.colorSelect.value = defaults.colorScheme;
    elements.scaleSelect.value = defaults.scale;
    elements.showGrid.checked = defaults.showGrid;
    
    const scheme = colorSchemes[defaults.colorScheme];
    renderer.setColors(scheme.bg, scheme.pixel, scheme.backlight);
    renderer.setScale(scaleMap[defaults.scale]);
    renderer.gridEnabled = defaults.showGrid;
    
    refreshDisplay();
    console.log('Reset to defaults');
}

// ==================== ЗАПУСК ПРИЛОЖЕНИЯ ====================

function init() {
    console.log('🚀 LCD1602 Generator starting...');
    
    // Проверяем зависимости
    if (!checkDependencies()) {
        console.error('Failed to initialize. Check console for errors.');
        return;
    }
    
    // Устанавливаем начальные значения
    resetToDefaults();
    
    // Первый рендер
    refreshDisplay();
    
    console.log('✅ Application ready');
    console.log('💡 Try typing in the text fields to see live preview');
}

// Запускаем приложение
init();

// Экспортируем для возможного использования в консоли
export { lcd, renderer, refreshDisplay, resetToDefaults };s