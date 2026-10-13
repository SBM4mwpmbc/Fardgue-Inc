#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cctype>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

namespace {
using Clock = std::chrono::steady_clock;

struct GameState {
    std::atomic<long long> fardpoints{0};
    std::atomic<long long> infected{0};
    std::atomic<long long> died{0};
    std::atomic<long long> cure{0};

    std::atomic<double> infectivity{2.0};
    std::atomic<double> severity{0.1};
    std::atomic<double> lethality{0.0};

    std::atomic<double> cureIntervalSeconds{15.0};
    std::atomic<bool> running{false};

    std::atomic<bool> drugResistanceUnlocked{false};
    std::atomic<bool> heatResistanceUnlocked{false};
    std::atomic<bool> coldResistanceUnlocked{false};
    std::atomic<bool> geneticHardeningUnlocked{false};

    std::atomic<bool> airParticlesUnlocked{false};
    std::atomic<bool> waterParticlesUnlocked{false};
    std::atomic<bool> livestockUnlocked{false};
    std::atomic<bool> bloodUnlocked{false};
    std::atomic<bool> insectsAndBirdsUnlocked{false};

    std::atomic<bool> insomniaUnlocked{false};
    std::atomic<bool> stomachacheUnlocked{false};
    std::atomic<bool> coughingUnlocked{false};
    std::atomic<bool> feverUnlocked{false};
    std::atomic<bool> tumoursUnlocked{false};
    std::atomic<bool> cardiacArrestUnlocked{false};
    std::atomic<bool> organFailureUnlocked{false};
    std::atomic<bool> excessiveDiarrheaUnlocked{false};
};

GameState g;
std::mutex outputMutex;
std::mutex wakeMutex;
std::condition_variable wakeCondition;

void printLine(const std::string& text) {
    std::lock_guard<std::mutex> lock(outputMutex);
    std::cout << text << '\n' << std::flush;
}

template <typename T>
void printValue(const std::string& label, const T& value) {
    std::lock_guard<std::mutex> lock(outputMutex);
    std::cout << label << value << '\n' << std::flush;
}

std::string trim(std::string text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

std::string lowercase(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return text;
}

std::string normalize(std::string text) {
    return lowercase(trim(std::move(text)));
}

bool waitOrStop(double seconds) {
    if (seconds < 0.001) seconds = 0.001;
    std::unique_lock<std::mutex> lock(wakeMutex);
    return wakeCondition.wait_for(
        lock,
        std::chrono::duration<double>(seconds),
        [] { return !g.running.load(); }
    );
}

void stopGame() {
    g.running = false;
    wakeCondition.notify_all();
}

double intervalFor(double value) {
    if (value >= 0.0 && value <= 3.0) return 20.0;
    if (value > 3.0 && value <= 10.0) return 15.0;
    if (value > 10.0 && value <= 20.0) return 12.0;
    if (value > 20.0 && value <= 30.0) return 10.0;
    if (value > 30.0 && value <= 40.0) return 8.0;
    if (value > 40.0 && value <= 50.0) return 6.0;
    if (value > 50.0 && value <= 60.0) return 3.0;
    if (value > 60.0 && value <= 70.0) return 1.0;
    if (value > 70.0 && value <= 80.0) return 0.5;
    if (value > 80.0 && value <= 90.0) return 0.2;
    if (value > 90.0 && value <= 100.0) return 0.1;
    if (value > 100.0) return 0.05;
    return -1.0;
}

void fardpointWorker() {
    while (g.running.load()) {
        if (waitOrStop(5.0)) break;
        if (!g.running.load()) break;
        g.fardpoints.fetch_add(3);
        printValue("Fardpoints ", g.fardpoints.load());
    }
}

void infectionWorker() {
    while (g.running.load()) {
        const double interval = intervalFor(g.infectivity.load());
        if (interval < 0.0) {
            printLine("UNKNOWN ERROR");
            if (waitOrStop(0.1)) break;
            continue;
        }
        if (waitOrStop(interval)) break;
        if (!g.running.load()) break;
        g.infected.fetch_add(20);
        printValue("Infected ", g.infected.load());
    }
}

void deathWorker() {
    while (g.running.load()) {
        const double interval = intervalFor(g.lethality.load());
        if (interval < 0.0) {
            printLine("UNKNOWN ERROR");
            if (waitOrStop(0.1)) break;
            continue;
        }
        if (waitOrStop(interval)) break;
        if (!g.running.load()) break;
        g.died.fetch_add(1);
        printValue("Died ", g.died.load());
    }
}

void cureWorker() {
    while (g.running.load()) {
        if (waitOrStop(g.cureIntervalSeconds.load())) break;
        if (!g.running.load()) break;
        g.cure.fetch_add(1);
        printValue("Cure ", g.cure.load());
    }
}

void milestoneWorker() {
    bool announcedNoHealthyPeople = false;
    bool announcedCureResearch = false;
    while (g.running.load()) {
        const long long infected = g.infected.load();
        const long long died = g.died.load();

        // These intentionally retain the source's exact-equality checks.
        if (!announcedNoHealthyPeople && infected == 7000000000LL) {
            printLine("There are no healthy people left in the world");
            printLine("Now try to kill everyone with your plague.");
            announcedNoHealthyPeople = true;
        }
        if (!announcedCureResearch && infected == 10000) {
            printLine("The cure is being researched.");
            printLine("You need to evolve your plague to make it stronger.");
            g.cureIntervalSeconds = 20.0;
            announcedCureResearch = true;
        }
        if (died == 7000000000LL) {
            printLine("Everyone is dead. You have won the game.");
            printLine("Congratulations!");
            stopGame();
            break;
        }
        if (waitOrStop(0.05)) break;
    }
}

bool buyUpgrade(
    const std::string& command,
    const std::string& name,
    long long cost,
    double severityGain,
    double infectivityGain,
    double lethalityGain,
    std::atomic<bool>& unlocked,
    const std::string& legacyAlias = {}
) {
    const std::string commandName = "evolve " + normalize(name);
    const std::string typed = normalize(command);
    const std::string legacy = legacyAlias.empty() ? std::string{} : "evolve " + normalize(legacyAlias);
    if (typed != commandName && (legacy.empty() || typed != legacy)) return false;

    // Match the source's order: check points before checking the unlock flag.
    if (g.fardpoints.load() < cost) {
        printLine("Not enough fardpoints!");
        return true;
    }
    if (unlocked.load()) {
        printLine("You already have that!");
        return true;
    }

    g.fardpoints.fetch_sub(cost);
    g.severity.store(g.severity.load() + severityGain);
    g.infectivity.store(g.infectivity.load() + infectivityGain);
    g.lethality.store(g.lethality.load() + lethalityGain);
    unlocked = true;
    printLine("Successfully evolved!");
    return true;
}

bool isSubmenuExit(const std::string& input) {
    const std::string cmd = normalize(input);
    return cmd == "exit";
}

void abilityMenu() {
    printLine("Here are some abilites.");
    printLine("Commands: EVOLVE <name> - Evolves an ability, making your plague stronger. EXIT - Exits this menu");
    printLine("Drug resistance - Antibiotics wont work, making your plague more powerful. Requires 10 fardpoints.");
    printLine("Heat resistance - Your plague will be able to resist the heat, good for hot climates. Requires 6 fardpoints.");
    printLine("Cold resistance - Your plague will be able to resist the cold, good for cold climates. Requires 7 fardpoints.");
    printLine("Genetic Hardening - Makes the cure research go slower. Requires 17 fardpoints.");

    std::string input;
    while (g.running.load() && std::getline(std::cin, input)) {
        if (buyUpgrade(input, "drug resistance", 10, 4, 2, 0, g.drugResistanceUnlocked)) continue;
        if (buyUpgrade(input, "heat resistance", 6, 2, 1, 0, g.heatResistanceUnlocked)) continue;
        if (buyUpgrade(input, "cold resistance", 7, 2, 1, 0, g.coldResistanceUnlocked)) continue;
        if (buyUpgrade(input, "genetic hardening", 17, 5, 4, 0, g.geneticHardeningUnlocked)) continue;
        if (isSubmenuExit(input)) { printLine("Exiting.."); printLine("Exited!"); return; }
        printLine("Not an ability!");
    }
}

void transmissionMenu() {
    printLine("Here are some transmission upgrades.");
    printLine("Commands: EVOLVE <name> - Evolves a transmission, making your plague stronger. EXIT - Exits this menu");
    printLine("Air particles - Your pathogen travels through air, increasing your infectivity. Requires 8 fardpoints.");
    printLine("Water Particles - Makes your pathogen travel through water, increasing your infectivity. - Requires 9 fardpoints.");
    printLine("Livestock - Your pathogen travels through livestock. Great through rural regions. Requires 11 fardpoints.");
    printLine("Blood - Your pathogen goes through blood, such as kissing, cross contamination, and more. Requires 17 fardpoints.");
    printLine("Insects and birds - Your pathogen travels through many insects and birds. Requires 21 fardpoints.");

    std::string input;
    while (g.running.load() && std::getline(std::cin, input)) {
        if (buyUpgrade(input, "air particles", 8, 2, 5, 0, g.airParticlesUnlocked)) continue;
        if (buyUpgrade(input, "water particles", 9, 2, 5, 0, g.waterParticlesUnlocked)) continue;
        if (buyUpgrade(input, "livestock", 11, 1, 3, 0, g.livestockUnlocked)) continue;
        if (buyUpgrade(input, "blood", 17, 5, 4, 0, g.bloodUnlocked)) continue;
        if (buyUpgrade(input, "insects and birds", 21, 5, 4, 0, g.insectsAndBirdsUnlocked)) continue;
        if (isSubmenuExit(input)) { printLine("Exiting.."); printLine("Exited!"); return; }
        printLine("Not an ability!");
    }
}

void symptomMenu() {
    printLine("Here are some symptom upgrades.");
    printLine("Commands: EVOLVE <name> - Evolves a symptom, making your plague stronger. EXIT - Exits this menu");
    printLine("Insomnia - Inability to sleep, making people irritated and less productive. Requires 3 fardpoints.");
    printLine("Stomachache - Makes the stomach really irritated, causing vomiting. Requires 3 fardpoints.");
    printLine("Coughing - self explanatory - Requires 3 fardpoints.");
    printLine("Fever - Your body gets really hot, and in some cases can cause death. Requires 4 fardpoints.");
    printLine("Tumours - Causes cancer, which can lead to painful deaths. Requires 15 fardpoints.");
    printLine("Dizziness - Causes nausea and brain irritatibility. Requires 7 fardpoints.");
    printLine("Coma - Your brain goes unconcious for a long time, and can result to death if not monitored. Requires 12 fardpoints.");
    printLine("Possible cardiac arrest - Your heart goes in panic mode, which can be fatal. Requires 15 fardpoints.");
    printLine("Organ failure - Causes your organs to fail, which is extremely lethal. Requires 20 fardpoints.");
    printLine("Excessive diarrhea - Causes yo bum bum to go bye bye and explode. Requires 11 fardpoints.");

    std::string input;
    while (g.running.load() && std::getline(std::cin, input)) {
        if (buyUpgrade(input, "insomnia", 3, 1, 2, 0, g.insomniaUnlocked)) continue;
        if (buyUpgrade(input, "stomachache", 3, 2, 2, 0, g.stomachacheUnlocked, "stomach ache")) continue;
        if (buyUpgrade(input, "coughing", 3, 3, 2, 0, g.coughingUnlocked)) continue;
        if (buyUpgrade(input, "fever", 4, 5, 4, 0.3, g.feverUnlocked)) continue;
        if (buyUpgrade(input, "tumours", 15, 30, 12, 18, g.tumoursUnlocked, "tumors")) continue;
        if (buyUpgrade(input, "possible cardiac arrest", 15, 30, 14, 20, g.cardiacArrestUnlocked)) continue;
        if (buyUpgrade(input, "organ failure", 20, 40, 25, 20, g.organFailureUnlocked)) continue;
        // Accept the displayed spelling and the older source typo.
        if (buyUpgrade(input, "excessive diarrhea", 11, 2, 7, 0, g.excessiveDiarrheaUnlocked, "excessive diaherra")) continue;
        // Dizziness and Coma are listed in the Fardgramming source, but have no upgrade branch there.
        if (isSubmenuExit(input)) { printLine("Exiting.."); printLine("Exited!"); return; }
        printLine("Not an ability!");
    }
}

void diseaseMenu() {
    printLine("What would you like to evolve?");
    printLine("1 - Transmission");
    printLine("2 - Symptoms");
    printLine("3 - Abilities");
    printLine("Exit");

    std::string input;
    while (g.running.load() && std::getline(std::cin, input)) {
        if (input == "1") transmissionMenu();
        else if (input == "2") symptomMenu();
        else if (input == "3") abilityMenu();
        else if (input == "Exit" || input == "exit") {
            printLine("Exiting disease menu..");
            printLine("Exited");
            return;
        } else {
            printLine("Unknown command");
        }
    }
}

void resetGameState() {
    g.fardpoints = 0;
    g.infected = 0;
    g.died = 0;
    g.cure = 0;
    g.infectivity = 2.0;
    g.severity = 0.1;
    g.lethality = 0.0;
    g.cureIntervalSeconds = 15.0;

    g.drugResistanceUnlocked = false;
    g.heatResistanceUnlocked = false;
    g.coldResistanceUnlocked = false;
    g.geneticHardeningUnlocked = false;
    g.airParticlesUnlocked = false;
    g.waterParticlesUnlocked = false;
    g.livestockUnlocked = false;
    g.bloodUnlocked = false;
    g.insectsAndBirdsUnlocked = false;
    g.insomniaUnlocked = false;
    g.stomachacheUnlocked = false;
    g.coughingUnlocked = false;
    g.feverUnlocked = false;
    g.tumoursUnlocked = false;
    g.cardiacArrestUnlocked = false;
    g.organFailureUnlocked = false;
    g.excessiveDiarrheaUnlocked = false;
}

void gameCasual() {
    resetGameState();
    printLine("Infected 0");
    printLine("Died 0");
    printLine("Cure 0");
    printValue("", g.fardpoints.load());
    printLine("Welcome to Fardgue Inc. You need to make the most EVIL bacteria ever, causing the ultimate plague.");
    printLine("Your plague has started in Sweden.");
    printLine("COMMANDS: Disease - Open the disease menu, you can use this to evolve. DiseaseExit - Exits the disease menu. EXIT - Exit the game.");

    // Fardgramming's be_patient 5 runs before the playable counters are initialized.
    std::this_thread::sleep_for(std::chrono::seconds(5));
    g.infected = 3;
    g.fardpoints = 4;
    printValue("", g.infected.load());
    g.running = true;

    std::thread points(fardpointWorker);
    std::thread infected(infectionWorker);
    std::thread deaths(deathWorker);
    std::thread cure(cureWorker);
    std::thread milestones(milestoneWorker);

    std::string input;
    while (g.running.load() && std::getline(std::cin, input)) {
        if (input == "Disease") diseaseMenu();
        if (input == "EXIT") stopGame();
    }

    stopGame();
    if (points.joinable()) points.join();
    if (infected.joinable()) infected.join();
    if (deaths.joinable()) deaths.join();
    if (cure.joinable()) cure.join();
    if (milestones.joinable()) milestones.join();
}

void gameRealistic() {
    // In the source these difficulty functions are declared but not implemented yet.
}

void gameBrutal() {
    // In the source these difficulty functions are declared but not implemented yet.
}

void gameHardcore() {
    // In the source these difficulty functions are declared but not implemented yet.
}

void loadCasual() { gameCasual(); printLine("Loaded. Ready to play!"); }
void loadRealistic() { gameRealistic(); printLine("Loaded. Ready to play!"); }
void loadBrutal() { gameBrutal(); printLine("Loaded. Ready to play!"); }
void loadHardcore() { gameHardcore(); printLine("Loaded. Ready to play!"); }

} // namespace

int main() {
    printLine("Welcome to Fardgue Inc.");
    printLine("What would you like to be today?");
    printLine("1 - A bacteria.");
    printLine("2 - A virus.");

    std::string choice;
    if (!std::getline(std::cin, choice)) return 0;

    if (choice == "1") {
        printLine("Loading..");
        printLine("What mode would you like to do today?");
        printLine("Casual - For people wanting a quick game. Nobody washes their hands, doctors dont go to work and sick people are given hugs");
        printLine("Realistic - For people who want to be realistic ahh. 67% of people wash their hands, doctors work normally, and sick people are ignored");
        printLine("BRUTAL - For people who is a speedrunner. Everyone washes their hands, research doctors work 24/7, and sick people are locked in prison");
        printLine("HARDCORE - For people who are tryhards. Everyone sanitizes and washes their hands at the same time, doctors work overtime, and sick people are sent to torture chambers.");

        std::string mode;
        if (!std::getline(std::cin, mode)) return 0;
        const std::string selectedMode = normalize(mode);
        if (selectedMode == "casual") {
            printLine("Loading..");
            loadCasual();
        } else if (selectedMode == "realistic") {
            printLine("Loading..");
            loadRealistic();
        } else if (selectedMode == "brutal") {
            printLine("Loading..");
            loadBrutal();
        } else if (selectedMode == "hardcore") {
            printLine("Loading..");
            loadHardcore();
        } else {
            printLine("Not a level.");
        }
    } else if (choice == "2") {
        printLine("That is not unlocked.");
    } else {
        printLine("what. that isnt even a mode ding dong");
    }
    return 0;
}
