from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
TRANSPORT = ROOT / "src/racechrono/racechrono_ble_transport.cpp"


class RaceChronoBleAdvertisingContractTest(unittest.TestCase):
    def test_advertisement_contains_service_and_visible_device_name(self):
        source = TRANSPORT.read_text(encoding="utf-8")
        begin = source.split("bool RaceChronoBleTransport::begin()", 1)[1]
        begin = begin.split("bool RaceChronoBleTransport::startAdvertising()", 1)[0]

        self.assertIn("advertising->addServiceUUID(kServiceUuid)", begin)
        self.assertIn(
            "advertising->setName(kDeviceName)",
            begin,
            "NimBLE 2.x does not advertise the init() device name automatically",
        )


if __name__ == "__main__":
    unittest.main()
