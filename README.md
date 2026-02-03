# Open Source Budweiser Red Light

This project is my recreation of the Budweiser Red Light, powered by the XIAO ESP32-S3. It uses the NHL's API to check the score in real time, and a single 2N7000 MOSFET to activate a light and motor.
---
Budweiser Red light - https://redlights-dev-us.myshopify.com/

$10 spinning light - https://www.amazon.com/dp/B0CZ8H3RSD?ref=ppx_yo2ov_dt_b_fed_asin_title&th=1

---
## TODO:
- [ ] Add website for changing configurations.
- [ ] Add delay after score is noticed and before it reacts in case there's a stream delay.
- [ ] Add more functions to easily add a screen. (ie. get player who scored the goal.)
- [ ] Update the GetTimeRemaining to be two seperate functions for period number and time remaining.
