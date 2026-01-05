# ESP32 Pedal PCB Design

KiCad project for a custom ESP32-based pedal PCB.

⚠️ **UNTESTED** - This is a design-only PCB that has not been manufactured or tested.

## Images

![Schematic](../docs/schema.png)
*Complete schematic diagram*

![PCB Routing](../docs/PCB_routing.png)
*PCB layout and routing*

![3D Render](../docs/3D_render.png)
*3D visualization of the PCB*

![FireBeetle 2 Reference](../docs/Firebeetle2.png)
*Reference: FireBeetle 2 ESP32-E board (for comparison)*

## Design Files

All design files are included in this directory:
- `esp32-pedal-pcb.kicad_pro` - KiCad project file
- `esp32-pedal-pcb.kicad_sch` - Schematic
- `esp32-pedal-pcb.kicad_pcb` - PCB layout
- `esp32-pedal-pcb.kicad_dru` - Design rules
- `esp32-pedal-pcb.rules` - Custom design rules
- `esp32-pedal-pcb.csv` - Bill of Materials (BOM)

## Notes

- KiCad 7.0 or later required
- Custom Espressif symbol and footprint libraries included in this directory
- Design files are tracked in git (temporary files excluded via `.gitignore`)
