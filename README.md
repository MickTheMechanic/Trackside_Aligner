# Trackside_Aligner
mobile 2d high accuracy wheel alignment stand using DIY/IOT products/breakouts

<a rel="license" href="http://creativecommons.org/licenses/by-nc/4.0/"><img alt="Creative Commons License" style="border-width:0" src="https://i.creativecommons.org/l/by-nc/4.0/88x31.png" /></a><br />This work is licensed under a <a rel="license" href="http://creativecommons.org/licenses/by-nc/4.0/">Creative Commons Attribution-NonCommercial 4.0 International License</a>.

<img src="https://i.ibb.co/QKnbk2M/wheelstand-2021-Sep-27-11-34-43-AM-000-Customized-View5034318090.png" alt="wheelstand-2021-Sep-27-11-34-43-AM-000-Customized-View5034318090" border="0">
<img src="https://i.ibb.co/Jsqz7Lw/wheelstand-2021-Sep-27-11-34-04-AM-000-Customized-View39679336781.png" alt="wheelstand-2021-Sep-27-11-34-04-AM-000-Customized-View39679336781" border="0">

# Frontend
an example of the front end to be displayed on a mobile device: 
https://codepen.io/mickthemechanic/pen/MWoBVgx

# Toe
Toe angle is calculated by measuring the distance between the left and right wheel at two points. Measurements are taken from two laser range finders: 
https://www.jrt-measure.com/distance-sensor-short-range/57657185.html

# Camber
Camber angle is calculated by taking measurements from an MPU9250 accelerometer mounted to the stands. The acceleromter measures the difference between the stand angle (Camber) and the static acceleration of the earth (gravity).
