#include "power.hpp"
#include <iostream>
#define CHECK(x) do { if (!(x)) { std::cerr << "Failed: " #x "\n"; return 1; } } while(0)
int main() {
    using namespace doze;
    Countdown c; CHECK(!c.due(10)); CHECK(c.remaining(10)==0);
    c.start(Action::Sleep, 2, false, 100); CHECK(c.remaining(100)==2); CHECK(c.remaining(1101)==1);
    CHECK(!c.due(2099)); CHECK(c.due(2100)); CHECK(!c.due(2101));
    c.start(Action::Restart, 60, true, 3000); CHECK(c.force); c.cancel(); CHECK(!c.due(99000));
    bool rejected=false; try { c.start(Action::Sleep,0,false,0); } catch (...) { rejected=true; } CHECK(rejected);
    rejected=false; try { c.start(Action::Sleep,360000,false,0); } catch (...) { rejected=true; } CHECK(rejected);
    c.start(Action::Shutdown,359999,false,0); CHECK(c.remaining(0)==359999);
    rejected=false; try { c.start(Action::Sleep,1,false,0); } catch (...) { rejected=true; } CHECK(rejected); c.cancel();
    CHECK(parse_action("shutdown")==Action::Shutdown); CHECK(parse_action("restart")==Action::Restart);
    CHECK(parse_action("sleep")==Action::Sleep); CHECK(parse_action("hibernate")==Action::Hibernate);
    rejected=false; try { parse_action("anything"); } catch (...) { rejected=true; } CHECK(rejected);
    auto caps=capabilities(); CHECK(!caps.hibernate || caps.hibernateSupported);
    std::cout << "OK: countdown boundaries, cancellation, duplicate prevention, actions and read-only capabilities. No power action executed.\n";
}
