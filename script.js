// --- STATE MANAGEMENT ---
const state = { currentSlide: 0, slides: [], isFocusMode: false };

// --- INIT ---
document.addEventListener("DOMContentLoaded", () => {
  state.slides = Array.from(document.querySelectorAll("section"));
  initTheme();
  initMiniMap();
  initDiagramInteractions();

  // INTERSECTION OBSERVER FOR SCROLL TRACKING
  const observer = new IntersectionObserver(
    (entries) => {
      entries.forEach((entry) => {
        if (entry.isIntersecting) {
          const index = state.slides.indexOf(entry.target);
          state.currentSlide = index;
          // Mark viewed to trigger animations
          entry.target.classList.add("active", "viewed");
          updateMiniMap(index);
        } else {
          entry.target.classList.remove("active");
        }
      });
    },
    { threshold: 0.5 },
  ); // Trigger when 50% visible

  state.slides.forEach((slide) => observer.observe(slide));

  document.addEventListener("keydown", handleInput);
});

// --- 1. NAV ENGINE ---
function handleInput(e) {
  if (e.key === "ArrowRight" || e.key === " " || e.key === "Enter") {
    e.preventDefault();
    if (state.currentSlide < state.slides.length - 1)
      scrollToSlide(state.currentSlide + 1);
  } else if (e.key === "ArrowLeft") {
    e.preventDefault();
    if (state.currentSlide > 0) scrollToSlide(state.currentSlide - 1);
  } else if (e.key === "f" || e.key === "F") {
    toggleFocusMode();
  }
}

function scrollToSlide(index) {
  state.slides[index].scrollIntoView({ behavior: "smooth" });
}

// --- 2. THEME ---
function initTheme() {
  setTheme(localStorage.getItem("theme") || "light");
}
function setTheme(theme) {
  document.documentElement.setAttribute("data-theme", theme);
  localStorage.setItem("theme", theme);
  document.querySelectorAll(".theme-btn").forEach((btn) => {
    btn.classList.remove("active");
    if (btn.getAttribute("onclick").includes(theme))
      btn.classList.add("active");
  });
}
function toggleFocusMode() {
  state.isFocusMode = !state.isFocusMode;
  document.body.classList.toggle("focus-mode", state.isFocusMode);
}

// --- 3. MINI MAP ---
function initMiniMap() {
  const mapContainer = document.getElementById("miniMap");
  state.slides.forEach((slide, index) => {
    const item = document.createElement("div");
    item.className = "map-item";
    item.innerText = slide.getAttribute("data-title");
    item.onclick = () => scrollToSlide(index);
    mapContainer.appendChild(item);
  });
}
function updateMiniMap(index) {
  document.querySelectorAll(".map-item").forEach((item, i) => {
    item.classList.toggle("active", i === index);
  });
}

// --- 4. DIAGRAM ---
function initDiagramInteractions() {
  const popup = document.getElementById("dynamic-info");
  const titleEl = document.getElementById("info-title");
  const descEl = document.getElementById("info-desc");
  document.querySelectorAll(".d-box").forEach((box) => {
    box.addEventListener("mouseenter", () => {
      const title = box.getAttribute("data-title");
      if (title) {
        titleEl.innerText = title;
        descEl.innerText = box.getAttribute("data-desc");
        popup.classList.add("active");
        popup.className = "info-popup active";
        if (box.classList.contains("d-sensor")) {
          popup.classList.add("left");
          popup.style.borderLeftColor =
            box.style.borderLeftColor || "var(--accent)";
        } else if (box.classList.contains("d-output")) {
          popup.classList.add("right");
          popup.style.borderLeftColor = "var(--alert)";
        } else {
          popup.classList.add("center");
          popup.style.borderLeftColor = "#1e40af";
        }
      }
    });
    box.addEventListener("mouseleave", () => popup.classList.remove("active"));
  });
}

// --- 5. SIMULATOR ---
const consoleDiv = document.getElementById("console");
function sim(msg, type) {
  const div = document.createElement("div");
  const time = new Date().toLocaleTimeString();
  div.innerHTML = `[${time}] ${msg === "SLOUCH" ? "⚠️ SLOUCH DETECTED" : "✅ SIP DETECTED"}`;
  div.style.color = type === "alert" ? "var(--alert)" : "var(--accent)";
  consoleDiv.appendChild(div);
  consoleDiv.scrollTop = consoleDiv.scrollHeight;
}
document.getElementById("connectBtn").addEventListener("click", async () => {
  if ("serial" in navigator) {
    try {
      const port = await navigator.serial.requestPort();
      await port.open({ baudRate: 9600 });
      sim("HARDWARE CONNECTED", "success");
      const textDecoder = new TextDecoderStream();
      const readableStreamClosed = port.readable.pipeTo(textDecoder.writable);
      const reader = textDecoder.readable.getReader();
      while (true) {
        const { value, done } = await reader.read();
        if (done) break;
        if (value) {
          if (value.includes("SLOUCH")) sim("SLOUCH", "alert");
          else if (value.includes("SIP")) sim("SIP", "success");
        }
      }
    } catch (err) {
      sim("Error: " + err, "alert");
    }
  } else {
    alert("Use Chrome/Edge/Brave");
  }
});
