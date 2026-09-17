# LVGL appearance approval prototype

Isolated, static 800×480 prototypes for Analog Style (9), Side Gear (5),
and Strip Style (4). Not compiled into the firmware and not a hardware test.
The LVGL 8.4 software renderer writes actual display framebuffers to PPM;
Pillow only converts their file format losslessly to PNG. GitHub Actions
performs compilation and rendering. No firmware source or saved configuration
is changed by this prototype. Await user approval before integrating.

Uses black panels, thin blue-grey outlines, cyan navigation, parameter-colored
rails, condensed Rajdhani Bold digits, fine scale ticks and slanted curved
segments. Strip has no logo and no large center-card outlines. Side Gear
uses a shared outer frame for RPM and speed. Analog uses a colored dial ring.
Temperature bars remain available; two examples are shown as slim bottom rails.
Static values are 6840 RPM, 137 km/h and gear 3. Maximum scale is 10000 RPM.
This prototype validates appearance only: no input, persistence, live refresh
or flashing behavior is implemented. Production rendering cost is not measured.
