#include "WorldCore.h"

#include <iostream>
#include <stdexcept>

using namespace thug;

namespace {

int checks = 0;
void Check(bool value, const char* message) {
  ++checks;
  if (!value) throw std::runtime_error(message);
}

save::WorldObjectDefinition Object(const char* id, const char* type,
                                    const char* state, double x) {
  return {id, type, {state, {x, 0, 0}, {}, true}};
}

struct Fixture {
  save::WorldState state{"hospital", {
      Object("C_SECURITY", "circuit", "on", 0),
      Object("C_SERVICE", "circuit", "off", 1),
      Object("D_F2_SERVER", "door", "closed", 2),
      Object("D_G_CONTROL", "door", "closed", 3),
      Object("D_F3_ISOLATION", "door", "closed", 4),
      Object("D_F3_EXAMS", "door", "locked", 5),
      Object("CAM_LOBBY", "camera", "online", 6),
      Object("CAM_GARAGE", "camera", "looped", 7),
      Object("H_F3_ISOLATION", "hide", "empty", 8)}};
  world::WorldCore core{state,
      {{"D_F2_SERVER", "F2_CENTRAL", "F2_SERVER", "C_SECURITY",
        world::DoorPower::RequiresPower},
       {"D_G_CONTROL", "G_FLOOR", "G_CONTROL", "C_SERVICE",
        world::DoorPower::ReleasesWithoutPower},
       {"D_F3_ISOLATION", "F3_HALL_W", "F3_ISOLATION", "",
        world::DoorPower::Manual},
       {"D_F3_EXAMS", "F3_EXAMS", "F3_WINDOW", "",
        world::DoorPower::Manual}},
      {{"CAM_LOBBY", "F2_CAMERAS", "F1_LOBBY", "C_SECURITY"},
       {"CAM_GARAGE", "F2_CAMERAS", "G_FLOOR", "C_SERVICE"}},
      {{"H_F3_ISOLATION", "F3_ISOLATION"}}};
};

void DynamicWorld() {
  Fixture f;
  Check(f.core.ReadCamera("CAM_LOBBY") &&
            f.core.ReadCamera("CAM_LOBBY")->live &&
            f.core.ReadCamera("CAM_LOBBY")->viewNode == "F1_LOBBY",
        "Live camera shows its authored view only");
  Check(!f.core.ReadCamera("CAM_GARAGE"), "Unpowered looped camera unavailable");
  Check(f.core.OpenDoor("D_F2_SERVER", 1).has_value() &&
            f.core.CanTraverse("D_F2_SERVER"), "Powered access opens");
  Check(f.core.SetCircuit("C_SECURITY", false, 2).has_value() &&
            !f.core.ReadCamera("CAM_LOBBY") &&
            !f.core.CanTraverse("D_F2_SERVER"), "Blackout removes live feed and powered access");
  Check(f.core.CloseDoor("D_F2_SERVER", 3) &&
            !f.core.OpenDoor("D_F2_SERVER", 4), "Closed door cannot reopen while unpowered");
  Check(f.core.OpenDoor("D_F2_SERVER", 4, true).has_value() &&
            f.core.CanTraverse("D_F2_SERVER") &&
            f.core.CloseDoor("D_F2_SERVER", 4.5),
        "Forced opening bypasses a dead actuator and can be closed manually");
  Check(f.core.OpenDoor("D_G_CONTROL", 5).has_value() &&
            f.core.CanTraverse("D_G_CONTROL"), "Fail-release access remains available");
  Check(f.core.SetCircuit("C_SERVICE", true, 6).has_value() &&
            f.core.ReadCamera("CAM_GARAGE") &&
            !f.core.ReadCamera("CAM_GARAGE")->live,
        "Restored energy does not make a looped image live");
  Check(f.core.SetCameraState("CAM_GARAGE", "online", 7) &&
            f.core.ReadCamera("CAM_GARAGE")->live,
        "Repairing camera restores only its authored view");
  Check(!f.core.OpenDoor("D_F3_EXAMS", 8), "Locked door needs valid authorization");
  Check(f.core.UnlockDoor("D_F3_EXAMS", 9) &&
            f.core.OpenDoor("D_F3_EXAMS", 10).has_value(),
        "Validated unlock permits opening");
  Check(f.core.EnterHide("H_F3_ISOLATION", 11) &&
            f.core.Occupied("H_F3_ISOLATION") &&
            !f.core.EnterHide("H_F3_ISOLATION", 12),
        "Hiding place tracks occupancy without telling Hunter");
  Check(f.core.LeaveHide("H_F3_ISOLATION", 13) &&
            !f.core.Occupied("H_F3_ISOLATION"), "Leaving frees hide place");
  Check(f.core.JamDoor("D_F3_ISOLATION", 14) &&
            !f.core.OpenDoor("D_F3_ISOLATION", 15),
        "Jammed entrance cannot be opened");
  Check(!f.core.SetCameraState("CAM_LOBBY", "magic", 16),
        "Unknown camera state is rejected");
  Check(f.state.Find("D_F3_ISOLATION")->stateId == "jammed" &&
            f.state.Find("C_SECURITY")->stateId == "off",
        "All transitions live in existing serializable WorldState");
}

} // namespace

int main() {
  try {
    DynamicWorld();
    std::cout << "WorldCore: " << checks << " checks passed\n";
  } catch (const std::exception& error) {
    std::cerr << "WorldCore failed after " << checks << " checks: "
              << error.what() << '\n';
    return 1;
  }
}
