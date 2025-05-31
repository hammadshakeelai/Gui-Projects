#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <vector>

using namespace std;

			//---------------------------//
			//       Weapon Class        //
			//---------------------------//

class Weapon {
public:
    string name;
    int baseDamage;
    int luckyMinDamage;
    int luckyMaxDamage;
    
    Weapon(string n, int d, int lmin, int lmax)
        : name(n), baseDamage(d), luckyMinDamage(lmin), luckyMaxDamage(lmax) {}
    /*
    
	Weapon ( string n , int d , int lmin , int lmax ) {
		name = n;
		basedamage = d;
		luckyMinDamage = lmin;
		luckyMaxDamage = lmax;
	}

	*/

    // Returns either the fixed base damage or a random damage within a range if lucky.
    int attack(bool isLucky) {
        if (isLucky)
            return luckyMinDamage + rand() % (luckyMaxDamage - luckyMinDamage + 1);
        else
            return baseDamage;
    }
};

			//---------------------------//
			//      Character Class      //
			//---------------------------//
class Character {
public:
    string name;
    int health;
    Weapon* weapon;
    
    Character(string n, int h, Weapon* w)
        : name(n), health(h), weapon(w) {}

    virtual int attack(bool isLucky) {
        return weapon->attack(isLucky);
    }

    void takeDamage(int dmg) {
        health -= dmg;
        if (health < 0)
            health = 0;
    }
};

			//---------------------------//
			//        Player Class       //
			//---------------------------//
// Adds luck, current stair, and an "attackPenalty" that will reduce the player's attack damage
// after a successful evade.
class Player : public Character {
public:
    bool isLucky;
    int currentStair;
    int attackPenalty;  // Penalty applied to the next attack after evasion

    Player(string n, int h, Weapon* w, bool lucky)
        : Character(n, h, w), isLucky(lucky), currentStair(0), attackPenalty(0) {}

    // Override the attack method to account for any penalty from a prior evasion.
    int attack(bool isLucky) override {
        int damage = weapon->attack(isLucky);
        if (attackPenalty > 0) {
            damage -= attackPenalty;
            if (damage < 0)
                damage = 0;
            cout << "(Your attack is weakened by a penalty of " << attackPenalty << " due to your evasion.)" << endl;
            attackPenalty = 0; // Reset penalty after it is applied.
        }
        return damage;
    }
};

			//---------------------------//
			//        Enemy Class        //
			//---------------------------//
class Enemy : public Character {
public:
    Enemy(string n, int h, Weapon* w)
        : Character(n, h, w) {}
};

			//---------------------------//
			//          Dice Class        //
			//---------------------------//
class Die {
public:
    int roll() {
        return 1 + rand() % 6;
    }
};

			//---------------------------//
			//         Map Class         //
			//---------------------------//
class Map {
public:
    int totalStairs;
    Map(int total) : totalStairs(total) {}
};

			//---------------------------//
			//     BattleArena Class     //
			//---------------------------//
			
// Now the battle turn lets the player choose whether to attack or to try an evade maneuver.
// If the player chooses to evade, a 50% chance will reduce the enemy's attack damage for this round
// and add a penalty for the player's next attack.
class BattleArena {
public:
    void startBattle(Player &player, Enemy &enemy) {
        cout << "\n=== Battle Arena ===" << endl;
        cout << "A wild " << enemy.name << " appears in the 3x3 arena!" << endl;
        
        // Battle loop continues until either combatant is defeated.
        while (player.health > 0 && enemy.health > 0) {
            cout << "\n[" << player.name << "'s Turn]" << endl;
            cout << "Choose your move:" << endl;
            cout << "1. Attack" << endl;
            cout << "2. Evade" << endl;
            cout << "Enter your choice: ";
            int playerChoice;
            cin >> playerChoice;
            
            if (playerChoice == 1) {
                // Player performs an attack.
                int damage = player.attack(player.isLucky);
                enemy.takeDamage(damage);
                cout << player.name << " attacks " << enemy.name << " for " 
                     << damage << " damage. (" << enemy.name << " Health: " 
                     << enemy.health << ")" << endl;
                
                if (enemy.health <= 0)
                    break;
                
                // Enemy's turn with a normal attack.
                cout << "\n[" << enemy.name << "'s Turn]" << endl;
                int enemyDamage = enemy.attack(false);
                player.takeDamage(enemyDamage);
                cout << enemy.name << " attacks " << player.name << " for " 
                     << enemyDamage << " damage. (" << player.name << " Health: " 
                     << player.health << ")" << endl;
            }
            else if (playerChoice == 2) {
                // Player opts to evade.
                cout << player.name << " attempts to evade!" << endl;
                int chance = rand() % 100;  // 0 to 99
                if (chance < 50) {  // 50% chance of successful evasion
                    cout << "Evasion successful! You dodge most of the enemy's attack." << endl;
                    int enemyDamage = enemy.attack(false) / 2;
                    cout << enemy.name << " attacks with reduced force for " 
                         << enemyDamage << " damage." << endl;
                    player.takeDamage(enemyDamage);
                    // The compromise: player's next attack will suffer a penalty.
                    player.attackPenalty = 2;
                } else {
                    cout << "Evasion failed! You couldn't dodge the enemy's attack." << endl;
                    int enemyDamage = enemy.attack(false);
                    cout << enemy.name << " attacks for " 
                         << enemyDamage << " damage." << endl;
                    player.takeDamage(enemyDamage);
                }
            }
            else {
                // If the input is invalid, default to a normal attack.
                cout << "Invalid choice. Defaulting to attack." << endl;
                int damage = player.attack(player.isLucky);
                enemy.takeDamage(damage);
                cout << player.name << " attacks " << enemy.name << " for " 
                     << damage << " damage. (" << enemy.name << " Health: " 
                     << enemy.health << ")" << endl;
                
                if (enemy.health <= 0)
                    break;
                
                cout << "\n[" << enemy.name << "'s Turn]" << endl;
                int enemyDamage = enemy.attack(false);
                player.takeDamage(enemyDamage);
                cout << enemy.name << " attacks " << player.name << " for " 
                     << enemyDamage << " damage. (" << player.name << " Health: " 
                     << player.health << ")" << endl;
            }
        }

        if (player.health > 0)
            cout << "\n" << player.name << " has defeated the " << enemy.name << "!" << endl;
        else
            cout << "\n" << player.name << " has been defeated by the " << enemy.name << "..." << endl;
    }
};

			//---------------------------//
			//  Function: displayStatus  //
			//---------------------------//
			
void displayStatus(const Player &player, const Map &map) {
    cout << "\n--- Status ---" << endl;
    cout << "Name: " << player.name << endl;
    cout << "Health: " << player.health << endl;
    cout << "Weapon: " << player.weapon->name << endl;
    cout << "Current Stair: " << player.currentStair << " / " << map.totalStairs << endl;
    cout << "Lucky Mode: " << (player.isLucky ? "On" : "Off") << endl;
    cout << "--------------" << endl;
}

			//---------------------------//
			//          Main             //
			//---------------------------//
			
int main() {
    srand(static_cast<unsigned>(time(0)));

    cout << "Welcome to the Action RPG Game!" << endl;
    cout << "Do you feel lucky? (yes/no): ";
    string choice;
    cin >> choice;
    bool isLucky = (choice == "yes" || choice == "y");

    // Get player's name.
    cout << "\nEnter your character's name: ";
    string playerName;
    getline(cin >> ws, playerName);
    if (playerName.empty()) {
        playerName = "Hero";  // Default name if none provided.
    }

    // Choose a weapon.
    cout << "\nChoose your weapon:" << endl;
    cout << "1. Basic Sword" << endl;
    cout << "2. Axe" << endl;
    int weaponChoice;
    cout << "Enter weapon number: ";
    cin >> weaponChoice;
    while (weaponChoice < 1 || weaponChoice > 2) {
        cout << "Invalid choice. Please choose 1 or 2: ";
        cin >> weaponChoice;
    }
    Weapon basicSword("Basic Sword", 10, 5, 20);
    Weapon axe("Axe", 12, 6, 22);
    Weapon* chosenWeapon = (weaponChoice == 2) ? &axe : &basicSword;

    // Create the player.
    Player player(playerName, 100, chosenWeapon, isLucky);

    // Initialize map and die.
    Map map(20);
    Die die;

    // Define a list of possible enemies.
    vector<Enemy> enemies;
    Weapon enemyClaw("Claw", 8, 4, 16);
    enemies.push_back(Enemy("Goblin", 50, &enemyClaw));
    Weapon enemyFang("Fang", 9, 3, 18);
    enemies.push_back(Enemy("Wolf", 60, &enemyFang));
    Weapon enemySmash("Smash", 11, 5, 20);
    enemies.push_back(Enemy("Orc", 70, &enemySmash));

    cout << "\nYour journey on the staircase begins!" << endl;
    cout << "Reach stair " << map.totalStairs << " to advance to the next level." << endl;

    // Main game loop.
    while (player.currentStair < map.totalStairs && player.health > 0) {
        cout << "\n========================" << endl;
        cout << "Main Menu:" << endl;
        cout << "1. Roll Die to Move" << endl;
        cout << "2. Check Status" << endl;
        cout << "3. Quit Game" << endl;
        cout << "Enter your choice: ";
        int mainChoice;
        cin >> mainChoice;
        while (mainChoice < 1 || mainChoice > 3) {
            cout << "Invalid choice. Please choose 1, 2, or 3: ";
            cin >> mainChoice;
        }
        if (mainChoice == 1) {
            // Roll the die and move.
            int rollVal = die.roll();
            cout << "\nYou rolled a " << rollVal << "!" << endl;
            player.currentStair += rollVal;
            if (player.currentStair > map.totalStairs)
                player.currentStair = map.totalStairs;
            cout << "You are now on stair " << player.currentStair 
                 << " of " << map.totalStairs << "." << endl;

            // On special stairs (divisible by 5), trigger a battle.
            if (player.currentStair % 5 == 0) {
                cout << "\nYou've landed on a special stair!" << endl;
                int enemyIndex = rand() % enemies.size();
                // Create a fresh copy of the enemy for this battle.
                Enemy battleEnemy = enemies[enemyIndex];

                BattleArena arena;
                arena.startBattle(player, battleEnemy);

                if (player.health <= 0) {
                    cout << "\nGame Over!" << endl;
                    break;
                } else {
                    cout << "\nYou survived the battle and continue your journey!" << endl;
                }
            }
        }
        else if (mainChoice == 2) {
            displayStatus(player, map);
        }
        else if (mainChoice == 3) {
            cout << "\nYou have chosen to quit the game. Goodbye!" << endl;
            break;
        }
        else {
            cout << "\nInvalid choice, please select a valid option." << endl;
        }
    }

    if (player.health > 0 && player.currentStair >= map.totalStairs)
        cout << "\nCongratulations, " << player.name 
             << "! You have reached the top of the staircase and advanced to the next level!" << endl;

    return 0;
}
