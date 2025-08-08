document.addEventListener('DOMContentLoaded', () => {

    const latitudeInput = document.getElementById('latitude');
    const longitudeInput = document.getElementById('longitude');
    const weatherLink = document.getElementById('weather-link');

    function updateWeatherLink() {
        const lat = latitudeInput.value;
        const lon = longitudeInput.value;

        const newUrl = `/weather?lat=${encodeURIComponent(lat)}&lon=${encodeURIComponent(lon)}`;

        weatherLink.href = newUrl;
    }

    latitudeInput.addEventListener('input', updateWeatherLink);
    longitudeInput.addEventListener('input', updateWeatherLink);

    updateWeatherLink();
});
