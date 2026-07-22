/**
 * Canonical ISL scripts for Mythar demo.
 */

export const ISL_OPENING_4D_REVEAL = `
intent play_timeline("opening_4d_reveal")
  in world("world-mythar-plains")
  at "00:00:00.000"
  with evidence("ev-open-4d-001");
`.trim();

export const ISL_RENDER_TESSERACT = `
intent render_4d_tesseract("tesseract-hero")
  in world("world-mythar-plains")
  with params { d4: 4.0, d3: 4.0, speed: 1.5 };
`.trim();
