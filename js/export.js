/**
 * Picture + movie export from the live canvas render.
 */

function stamp() {
  const d = new Date();
  const p = (n) => String(n).padStart(2, "0");
  return `${d.getFullYear()}${p(d.getMonth() + 1)}${p(d.getDate())}-${p(d.getHours())}${p(d.getMinutes())}${p(d.getSeconds())}`;
}

function downloadBlob(blob, filename) {
  const url = URL.createObjectURL(blob);
  const a = document.createElement("a");
  a.href = url;
  a.download = filename;
  a.click();
  URL.revokeObjectURL(url);
}

export { downloadBlob, stamp };

/** Save current canvas frame as a PNG picture. */
export function capturePicture(canvas, basename = "4dce-frame") {
  return new Promise((resolve, reject) => {
    canvas.toBlob(
      (blob) => {
        if (!blob) {
          reject(new Error("Canvas export failed"));
          return;
        }
        const s = stamp();
        const name = `${basename}-${s}.png`;
        downloadBlob(blob, name);
        resolve({ filename: name, bytes: blob.size, stamp: s });
      },
      "image/png",
    );
  });
}

/** Download a JSON provenance sidecar next to an artifact. */
export function downloadProvenance(provenance, basename = "4dce-provenance") {
  const name = `${basename}-${stamp()}.json`;
  const blob = new Blob([JSON.stringify(provenance, null, 2)], {
    type: "application/json",
  });
  downloadBlob(blob, name);
  return { filename: name, bytes: blob.size };
}

function pickMimeType() {
  const candidates = [
    "video/webm;codecs=vp9",
    "video/webm;codecs=vp8",
    "video/webm",
  ];
  for (const type of candidates) {
    if (typeof MediaRecorder !== "undefined" && MediaRecorder.isTypeSupported(type)) {
      return type;
    }
  }
  return "";
}

/**
 * Record the canvas animation into a WebM movie file.
 * Uses MediaRecorder on canvas.captureStream.
 */
export function recordMovie(canvas, options = {}) {
  const seconds = options.seconds ?? 8;
  const fps = options.fps ?? 30;
  const basename = options.basename ?? "4dce-movie";
  const onProgress = options.onProgress ?? null;

  const mimeType = pickMimeType();
  if (!mimeType) {
    return Promise.reject(
      new Error("This browser cannot record WebM from canvas"),
    );
  }

  const stream = canvas.captureStream(fps);
  const chunks = [];
  const recorder = new MediaRecorder(stream, {
    mimeType,
    videoBitsPerSecond: options.bitRate ?? 5_000_000,
  });

  recorder.ondataavailable = (e) => {
    if (e.data && e.data.size > 0) chunks.push(e.data);
  };

  return new Promise((resolve, reject) => {
    let tick = null;
    let startedAt = 0;

    recorder.onerror = (e) => {
      if (tick) clearInterval(tick);
      reject(e.error || new Error("Recording failed"));
    };

    recorder.onstop = () => {
      if (tick) clearInterval(tick);
      stream.getTracks().forEach((t) => t.stop());
      const blob = new Blob(chunks, { type: mimeType.split(";")[0] });
      const name = `${basename}-${stamp()}.webm`;
      downloadBlob(blob, name);
      resolve({ filename: name, bytes: blob.size, seconds, fps });
    };

    recorder.start(250);
    startedAt = performance.now();

    tick = setInterval(() => {
      const elapsed = (performance.now() - startedAt) / 1000;
      if (onProgress) onProgress(Math.min(1, elapsed / seconds));
      if (elapsed >= seconds) {
        clearInterval(tick);
        tick = null;
        if (recorder.state !== "inactive") recorder.stop();
      }
    }, 100);
  });
}
