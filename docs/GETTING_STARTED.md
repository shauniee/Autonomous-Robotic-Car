# Getting started

1. Read the [design review](<../../CSC01 Design Review.docx>), [project requirements](project-info.md), [architecture](ARCHITECTURE.md) and [project plan](project-plan.md).
2. Check [hardware](HARDWARE.md) and [technical evidence](tech-info.md) for Robo Pico wiring, electrical limits and the submitted/current IR-port difference.
3. For the current robot, use [right IR operation](RIGHT_IR_LINE_FOLLOW.md), flash `right_ir_follow_mqtt`, capture black/white in the MQTT dashboard, then start locally with GP20. GP21 stops.
4. Use [implementation](IMPLEMENTATION.md) to follow source files and understand what is still a scaffold. The full mission’s proposed FreeRTOS tasks are not yet implemented.
5. Record actual calibration and planned T01–T16 measurements in [test-log.csv](test-log.csv). Builds/fake-SDK checks are not physical trials.

Owners: Nicholas integration/Safety/Mission/line, Mahesha motion, Darrius terrain, Jia Jing obstacle, Shaun communications. Target controls are local; MQTT status requests never drive the car. The current bench also accepts stopped sensor calibration.

Broker/schema deployment, exact installed board variants, measured wheel geometry, course barcode mapping/finish rules, hump-height references and timing/resource budgets still require verification. The configured Mac bench does not settle every integrated deployment choice.
