# Trackside_Aligner
mobile 2d high accuracy wheel alignment stand using DIY/IOT products/breakouts
<img src="https://i.ibb.co/QKnbk2M/wheelstand-2021-Sep-27-11-34-43-AM-000-Customized-View5034318090.png" alt="wheelstand-2021-Sep-27-11-34-43-AM-000-Customized-View5034318090" border="0">
<img src="https://i.ibb.co/Jsqz7Lw/wheelstand-2021-Sep-27-11-34-04-AM-000-Customized-View39679336781.png" alt="wheelstand-2021-Sep-27-11-34-04-AM-000-Customized-View39679336781" border="0">

# Toe
Toe angle is calculated by measuring the distance between the left and right wheel at two points. Measurements are taken from two vl53l1 TOF (Time of Flight) sensors. 
## documentation
https://www.st.com/en/imaging-and-photonics-solutions/vl53l1.html#documentation
## API
https://github.com/pololu/vl53l1x-st-api-arduino
https://www.st.com/en/embedded-software/stsw-img007.html

# Camber
Camber angle is calculated by taking measurements from an MPU9250 accelerometer mounted to the stands. The acceleromter measures the difference between the stand angle (Camber) and the static acceleration of the earth (gravity).
