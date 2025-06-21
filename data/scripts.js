// Updates the numeric output value for the main brightness slider
function updateBrightnessOutput(val) {
  const el = document.getElementById('brightnessValue');
  if (el) el.textContent = val;
}

// Enables or disables the auto-brightness fields based on the checkbox
function toggleAutoBrtFields() {
  const checkbox = document.getElementById("autoBrtEnabled");
  const hourFields = document.querySelectorAll("#autoBrtStartHr, #autoBrtEndHr");
  hourFields.forEach(el => el.disabled = !checkbox.checked);

  const autoSlider = document.getElementById('autoBrtLvl');
  const autoOutput = document.getElementById('autoBrtLvlOutput');
  if (autoSlider) autoSlider.disabled = !checkbox.checked;
  if (autoOutput) autoOutput.disabled = !checkbox.checked;
}

// Updates the double-hour range slider
function updateHourSlider() {
  const minVal = document.getElementById('autoBrtStartHr');
  const maxVal = document.getElementById('autoBrtEndHr');
  const sliderTrack = document.getElementById('slider-track');
  const startLabel = document.getElementById('rangeStartLabel');
  const endLabel = document.getElementById('rangeEndLabel');
  if (!minVal || !maxVal || !sliderTrack || !startLabel || !endLabel) return;

  let min = parseInt(minVal.value);
  let max = parseInt(maxVal.value);

  // Prevent the handles from getting too close (minimum gap of 1 hour)
  if (min >= max) {
    if (minVal === document.activeElement) {
      max = min + 1;
      if (max > parseInt(maxVal.max)) {
        max = parseInt(maxVal.max);
        min = max - 1;
      }
    } else {
      min = max - 1;
      if (min < parseInt(minVal.min)) {
        min = parseInt(minVal.min);
        max = min + 1;
      }
    }
    minVal.value = min;
    maxVal.value = max;
  }

  // Update the numeric labels
  startLabel.textContent = `${min}:00`;
  endLabel.textContent = `${max}:00`;

  // Visually fill the track between both handles
  const minLimit = parseInt(minVal.min);
  const maxLimit = parseInt(maxVal.max);
  const range = maxLimit - minLimit;
  const minPercent = ((min - minLimit) / range) * 100;
  const maxPercent = ((max - minLimit) / range) * 100;
  sliderTrack.style.left = minPercent + '%';
  sliderTrack.style.width = (maxPercent - minPercent) + '%';
}

// Set z-index so the dragged slider handle is always on top
function setActiveZIndex(which) {
  const minVal = document.getElementById('autoBrtStartHr');
  const maxVal = document.getElementById('autoBrtEndHr');
  if (!minVal || !maxVal) return;
  if (which === 'min') {
    minVal.style.zIndex = 10;
    maxVal.style.zIndex = 9;
  } else {
    minVal.style.zIndex = 9;
    maxVal.style.zIndex = 10;
  }
}

// Main UI initialization: set up all event listeners and initial state
document.addEventListener("DOMContentLoaded", () => {
  toggleAutoBrtFields();

  // Main brightness slider live output update
  const brightnessSlider = document.getElementById('brightness');
  const brightnessOutput = document.getElementById('brightnessValue');
  if (brightnessSlider && brightnessOutput) {
    brightnessOutput.textContent = brightnessSlider.value;
    brightnessSlider.oninput = function() {
      brightnessOutput.textContent = this.value;
    };
  }

  // Auto-brightness slider live output update
  const autoSlider = document.getElementById('autoBrtLvl');
  const autoOutput = document.getElementById('autoBrtLvlOutput');
  if (autoSlider && autoOutput) {
    autoOutput.textContent = autoSlider.value;
    autoSlider.oninput = function() {
      autoOutput.textContent = this.value;
    };
  }

  updateHourSlider();

  const minVal = document.getElementById('autoBrtStartHr');
  const maxVal = document.getElementById('autoBrtEndHr');
  if (minVal && maxVal) {
    minVal.addEventListener('input', updateHourSlider);
    maxVal.addEventListener('input', updateHourSlider);
    minVal.addEventListener('pointerdown', () => setActiveZIndex('min'));
    maxVal.addEventListener('pointerdown', () => setActiveZIndex('max'));

    // Ensure fields are enabled before form submission
    const form = document.querySelector("form");
    form.addEventListener("submit", () => {
      const fields = document.querySelectorAll("#autoBrtStartHr, #autoBrtEndHr, #autoBrtLvl");
      fields.forEach(el => el.disabled = false);
    });
  }
});