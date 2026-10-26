// Браузер (W1): до кода Emscripten — и на странице, и в воркерах потоков. Стек ошибки wasm — длиннее десяти кадров
Error.stackTraceLimit = 64;
// Ошибка в воркере (кадр, поток программы) доходит до страницы одной строкой — стек печатает сам воркер
if (typeof window === "undefined") {
  self.addEventListener("error", (e) => console.warn("worker error: " + (e.error && e.error.stack ? e.error.stack : e.message)));
}

// Звук (W2). SDL2 2.32 (порт Emscripten) спрашивает частоту AudioContext обычным EM_ASM — в воркере программы, где
// его объекта SDL2 нет (создаёт его главный поток страницы), и падает. Частота — одна везде: контекст страницы
// создаётся с 48 000 Гц, а воркеру SDL2 отвечаем ею же
var medecompAudioRate = 48000;
if (typeof window === "undefined") {
  Module["SDL2"] = Module["SDL2"] || { audioContext: { sampleRate: medecompAudioRate } };
} else if (typeof AudioContext !== "undefined") {
  const PageAudioContext = AudioContext;
  window.AudioContext = class extends PageAudioContext {
    constructor(options) { super(Object.assign({ sampleRate: medecompAudioRate }, options)); }
  };
}
