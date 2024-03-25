#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/system.hpp>

using namespace eosio;

CONTRACT mycontract : public contract {
public:
    using contract::contract;

   // Table pour stocker les informations des owners
   TABLE owner_struct {
      name owner_address;             // Adresse du propriétaire
      std::vector<uint64_t> land_ids;      // Liste des identifiants de la terre
      uint64_t totalDefense;
      uint64_t totalDefenseArm;
      uint64_t totalAttack;
      uint64_t totalAttackArm;
      uint64_t totalMoveCost;
      uint64_t numberofland; 

      uint64_t primary_key() const { return owner_address.value; }
   };

    // Table pour stocker les informations des players
    TABLE player_struct {
        name player_address;       // Adresse du joueur
        uint64_t totalDefense;     // Score total de défense
        uint64_t totalDefenseArm;  // Score total de défense avec armement
        uint64_t totalAttack;      // Score total d'attaque
        uint64_t totalAttackArm;   // Score total d'attaque avec armement
        uint64_t totalMoveCost;    // Coût total de déplacement

        auto primary_key() const { return player_address.value; }
    };

   TABLE support_struct {
      name owner_address;
      std::vector<name> supporters;
      uint64_t total_defense_score;
      uint64_t total_attack_score;
      uint64_t totalMoveCost; // Ensure this line correctly matches the field name used in upsupport

      uint64_t primary_key() const { return owner_address.value; }
   };



    TABLE forge_struct {
        name player_address;  // Adresse du joueur
        
        auto primary_key() const { return player_address.value; }
    };

    TABLE chest_struct {
        uint64_t land_id;       // Identifiant unique de la terre
        name owner;             // Propriétaire actuel
        uint64_t chest_level;   // Niveau du coffre
        uint64_t TLM;           // Quantité de TLM représentée comme un entier

        uint64_t primary_key() const { return land_id; }
    };

   TABLE mission_struct {
      name mission_name;
      uint64_t target_attack_points;
      asset reward;
      bool is_completed = false;
      uint64_t total_attack_points = 0;
      uint64_t last_hardening_time; // Ajout pour suivre le dernier durcissement
      bool is_distributed = false;
      time_point_sec deadline; // Ajout du membre 'deadline'

      uint64_t primary_key() const { return mission_name.value; }
   };



   TABLE player_mission_struct {
      uint64_t id; // Clé primaire unique
      name mission_name;
      name player;
      uint64_t attack_points;
      uint64_t last_participation_time;

      uint64_t primary_key() const { return id; }
      uint64_t by_player() const { return player.value; } // Clé secondaire pour le joueur
      uint64_t by_mission() const { return mission_name.value; } // Nouvelle clé secondaire pour la mission
   };

   TABLE member_struct {
      name player_name;

      uint64_t primary_key() const { return player_name.value; }
   };


   typedef multi_index<"members"_n, member_struct> member_table;
   typedef eosio::multi_index<"playermiss"_n, player_mission_struct,
      indexed_by<"byplayer"_n, const_mem_fun<player_mission_struct, uint64_t, &player_mission_struct::by_player>>,
      indexed_by<"bymission"_n, const_mem_fun<player_mission_struct, uint64_t, &player_mission_struct::by_mission>>
   > player_missions_table;
    typedef eosio::multi_index<"missions"_n, mission_struct> missions_table;
    typedef multi_index<"owners"_n, owner_struct> owners_table;
    typedef multi_index<"players"_n, player_struct> players_table;
    typedef multi_index<"supports"_n, support_struct> supports_table;
    typedef multi_index<"forge"_n, forge_struct> forge_table;
    typedef multi_index<"chests"_n, chest_struct> chests_table;

   struct OwnerDetails {
      name owner; 
      uint64_t land_id; // Supposons que chaque propriétaire ait un seul terrain à ajouter ou à mettre à jour
      uint64_t totalDefense; 
      uint64_t totalDefenseArm; 
      uint64_t totalAttack; 
      uint64_t totalAttackArm; 
      uint64_t totalMoveCost;
      // Pas besoin de numberofland ici, car il est dérivé du nombre d'éléments dans land_ids
   };

   ACTION addowners(std::vector<OwnerDetails> owner_details) {
    require_auth(get_self());

    owners_table _owners(get_self(), get_self().value);

    for (const auto& detail : owner_details) {
        auto owner_itr = _owners.find(detail.owner.value);

        if (owner_itr == _owners.end()) {
            // Si le propriétaire n'existe pas, créez un nouvel enregistrement
            _owners.emplace(get_self(), [&](auto& row) {
                row.owner_address = detail.owner;
                row.land_ids.push_back(detail.land_id); // Ajoutez land_id à la liste
                row.totalDefense = detail.totalDefense;
                row.totalDefenseArm = detail.totalDefenseArm;
                row.totalAttack = detail.totalAttack;
                row.totalAttackArm = detail.totalAttackArm;
                row.totalMoveCost = detail.totalMoveCost;
                row.numberofland = 1; // Initialiser à 1 car un nouveau terrain est ajouté
            });
        } else {
            // Si le propriétaire existe déjà, mettez à jour l'enregistrement existant
            _owners.modify(owner_itr, get_self(), [&](auto& row) {
                if (std::find(row.land_ids.begin(), row.land_ids.end(), detail.land_id) == row.land_ids.end()) {
                    row.land_ids.push_back(detail.land_id); // Ajoutez le terrain uniquement s'il est nouveau
                    row.numberofland += 1; // Incrémentez le nombre de terrains
                }
                // Mise à jour des autres champs
                row.totalDefense = detail.totalDefense;
                row.totalDefenseArm = detail.totalDefenseArm;
                row.totalAttack = detail.totalAttack;
                row.totalAttackArm = detail.totalAttackArm;
                row.totalMoveCost = detail.totalMoveCost;
            });
        }

        // Appeler upsupport pour chaque propriétaire mis à jour
        upsupport(detail.owner);
    }
}


   ACTION modifyowner(name owner, std::optional<uint64_t> land_id, std::optional<uint64_t> totalDefense, std::optional<uint64_t> totalDefenseArm,
                     std::optional<uint64_t> totalAttack, std::optional<uint64_t> totalAttackArm, std::optional<uint64_t> totalMoveCost) {
      require_auth(get_self());

      owners_table owners(get_self(), get_self().value);
      auto it = owners.find(owner.value);

      // Si le propriétaire n'existe pas, on l'ajoute
      if (it == owners.end()) {
         check(land_id.has_value(), "Land ID must be provided for new owner");
         owners.emplace(get_self(), [&](auto& row) {
               row.owner_address = owner;
               row.land_ids.push_back(land_id.value()); // Ajoute le premier terrain
               row.totalDefense = totalDefense.value_or(0);
               row.totalDefenseArm = totalDefenseArm.value_or(0);
               row.totalAttack = totalAttack.value_or(0);
               row.totalAttackArm = totalAttackArm.value_or(0);
               row.totalMoveCost = totalMoveCost.value_or(0);
               row.numberofland = 1; // Définit le nombre de terrains à 1 pour un nouveau propriétaire
         });
      } else {
         // Mise à jour du propriétaire existant
         owners.modify(it, get_self(), [&](auto& row) {
               // Ajouter un nouveau terrain uniquement si un land_id est fourni et n'est pas déjà présent
               if (land_id.has_value() && std::find(row.land_ids.begin(), row.land_ids.end(), land_id.value()) == row.land_ids.end()) {
                  row.land_ids.push_back(land_id.value());
                  row.numberofland++; // Incrémenter uniquement si un nouveau terrain est ajouté
               }
               // Mise à jour des autres attributs s'ils sont spécifiés
               if (totalDefense.has_value()) row.totalDefense = totalDefense.value();
               if (totalDefenseArm.has_value()) row.totalDefenseArm = totalDefenseArm.value();
               if (totalAttack.has_value()) row.totalAttack = totalAttack.value();
               if (totalAttackArm.has_value()) row.totalAttackArm = totalAttackArm.value();
               if (totalMoveCost.has_value()) row.totalMoveCost = totalMoveCost.value();
         });
      }
      upsupport(owner); // Mettre à jour les supports associés au propriétaire
   }


      // Action pour retirer un terrain d'un propriétaire
   ACTION removeland(name owner, uint64_t land_id) {
      require_auth(get_self());

      owners_table owners(get_self(), get_self().value);
      auto it = owners.find(owner.value);
      check(it != owners.end(), "Owner not found");

      owners.modify(it, get_self(), [&](auto& row) {
         auto itr = std::find(row.land_ids.begin(), row.land_ids.end(), land_id);
         check(itr != row.land_ids.end(), "Land not found for this owner");
         row.land_ids.erase(itr);

         // Si le propriétaire n'a plus de terrain, le supprimer de la table
         if (row.land_ids.empty()) {
               it = owners.erase(it);
         }
      });
   }

      struct  PlayerDetails {
      name player_address;
      uint64_t totalDefense;
      uint64_t totalDefenseArm;
      uint64_t totalAttack;
      uint64_t totalAttackArm;
      uint64_t totalMoveCost;
   };

   ACTION addplayers(const std::vector<PlayerDetails>& players_details) {
      require_auth(get_self());

      players_table _players(get_self(), get_self().value);

      for (const auto& details : players_details) {
         auto itr = _players.find(details.player_address.value);

         if (itr == _players.end()) {
            // Ajoute le joueur à la table s'il n'existe pas déjà
            _players.emplace(get_self(), [&](auto& new_player) {
               new_player.player_address = details.player_address;
               new_player.totalDefense = details.totalDefense;
               new_player.totalDefenseArm = details.totalDefenseArm;
               new_player.totalAttack = details.totalAttack;
               new_player.totalAttackArm = details.totalAttackArm;
               new_player.totalMoveCost = details.totalMoveCost;
            });
         } else {
            // Met à jour les informations du joueur s'il existe déjà
            _players.modify(itr, get_self(), [&](auto& existing_player) {
               existing_player.totalDefense = details.totalDefense;
               existing_player.totalDefenseArm = details.totalDefenseArm;
               existing_player.totalAttack = details.totalAttack;
               existing_player.totalAttackArm = details.totalAttackArm;
               existing_player.totalMoveCost = details.totalMoveCost;
            });
         }
         // Appeler la fonction upsupport pour chaque joueur mis à jour
         upsupport(details.player_address);
      }
   }



   // Action pour ajouter ou mettre à jour un joueur dans la table
   ACTION addplayer(name player_address, uint64_t totalDefense, uint64_t totalDefenseArm, 
                           uint64_t totalAttack, uint64_t totalAttackArm, uint64_t totalMoveCost) {
      require_auth(get_self());

      players_table players(get_self(), get_self().value);
      auto itr = players.find(player_address.value);

      if (itr == players.end()) {
         // Ajoute le joueur à la table s'il n'existe pas déjà
         players.emplace(get_self(), [&](auto& player) {
               player.player_address = player_address;
               player.totalDefense = totalDefense;
               player.totalDefenseArm = totalDefenseArm;
               player.totalAttack = totalAttack;
               player.totalAttackArm = totalAttackArm;
               player.totalMoveCost = totalMoveCost;
         });
      } else {
         // Met à jour les informations du joueur s'il existe déjà
         players.modify(itr, get_self(), [&](auto& row) {
               row.totalDefense = totalDefense;
               row.totalDefenseArm = totalDefenseArm;
               row.totalAttack = totalAttack;
               row.totalAttackArm = totalAttackArm;
               row.totalMoveCost = totalMoveCost;
         });
      }
      upsupport(player_address);
   }


      // Action pour ajouter un coffre à la table
   ACTION addchest(uint64_t land_id, name owner, uint64_t chest_level, uint64_t TLM) {
      require_auth(get_self());

      // Vérifie si le coffre existe déjà
      chests_table chests(get_self(), get_self().value);
      auto itr = chests.find(land_id);
      check(itr == chests.end(), "Chest already exists");

      // Ajoute le coffre à la table
      chests.emplace(get_self(), [&](auto& chest) {
         chest.land_id = land_id;
         chest.owner = owner;
         chest.chest_level = chest_level;
         chest.TLM = TLM;
      });
   }


   ACTION modifychest(uint64_t land_id, std::optional<name> new_owner, std::optional<uint64_t> new_level, std::optional<uint64_t> new_tlm, std::optional<uint64_t> tlm_to_withdraw) {
      require_auth(get_self());

      chests_table chests(get_self(), get_self().value);
      auto itr = chests.find(land_id);
      check(itr != chests.end(), "Chest not found");

      // Modification des propriétés du coffre
      chests.modify(itr, get_self(), [&](auto& chest) {
         if (new_owner.has_value()) chest.owner = new_owner.value(); // Vérifie si le nouveau propriétaire est fourni
         if (new_level.has_value()) chest.chest_level = new_level.value(); // Vérifie si le nouveau niveau est fourni
         if (new_tlm.has_value()) chest.TLM = new_tlm.value(); // Vérifie si le nouveau TLM est fourni
         if (tlm_to_withdraw.has_value()) {
               // Vérifier que le montant à retirer ne dépasse pas le montant disponible
               check(chest.TLM >= tlm_to_withdraw.value(), "Insufficient TLM in chest");
               chest.TLM -= tlm_to_withdraw.value(); // Diminuer le montant de TLM
         }
      });
   }

   ACTION addsupport(name player, name new_owner) {
      require_auth(player);

      // Vérifier si le nouveau propriétaire existe
      owners_table owners(get_self(), get_self().value);
      auto new_owner_itr = owners.find(new_owner.value);
      check(new_owner_itr != owners.end(), "New owner not found");

      // Récupérer les informations du joueur
      players_table players(get_self(), get_self().value);
      auto player_itr = players.find(player.value);
      check(player_itr != players.end(), "Player not found");

      // Vérifier si le propriétaire ne se soutient pas lui-même
      check(new_owner_itr->owner_address != player, "Owner cannot support itself");

      // Vérifier si le joueur est dans la forge
      forge_table forge(get_self(), get_self().value);
      auto forge_itr = forge.find(player.value);
      bool in_forge = forge_itr != forge.end();

      // Calculer les scores du joueur en fonction de s'il est dans la forge ou non
      uint64_t player_defense_score = in_forge ? player_itr->totalDefenseArm : player_itr->totalDefense;
      uint64_t player_attack_score = in_forge ? player_itr->totalAttackArm : player_itr->totalAttack;
      uint64_t player_move_cost = player_itr->totalMoveCost;

      // Récupérer la table des soutiens
      supports_table supports(get_self(), get_self().value);

      // Retirer le joueur des soutiens de l'ancien propriétaire
      for (auto& support : supports) {
         auto itr = std::find(support.supporters.begin(), support.supporters.end(), player);
         if (itr != support.supporters.end()) {
               supports.modify(support, get_self(), [&](auto& s) {
                  s.total_defense_score -= player_defense_score;
                  s.total_attack_score -= player_attack_score;
                  s.totalMoveCost -= player_move_cost;
                  s.supporters.erase(itr);
               });
               break;
         }
      }

      // Ajouter le joueur au nouveau propriétaire
      auto new_support_itr = supports.find(new_owner.value);
      if (new_support_itr == supports.end()) {
         supports.emplace(player, [&](auto& support) {
               support.owner_address = new_owner;
               support.supporters.push_back(player);
               support.total_defense_score = player_defense_score + new_owner_itr->totalDefense;
               support.total_attack_score = player_attack_score + new_owner_itr->totalAttack;
               support.totalMoveCost = player_move_cost + new_owner_itr->totalMoveCost;
         });
      } else {
         supports.modify(new_support_itr, get_self(), [&](auto& support) {
               support.supporters.push_back(player);
               support.total_defense_score += player_defense_score;
               support.total_attack_score += player_attack_score;
               support.totalMoveCost += player_move_cost;
         });
      }
   }



   ACTION createmis(name mission_name, uint64_t target_attack_points, asset reward, uint32_t deadline_seconds) {
      require_auth(get_self());

      // Obtenez le temps actuel en secondes depuis l'époque
      uint32_t current_time_seconds = current_time_point().sec_since_epoch();

      // Calculez la date limite en ajoutant le délai spécifié
      uint32_t mission_deadline = current_time_seconds + deadline_seconds;

      missions_table missions(get_self(), get_self().value);
      auto existing_mission = missions.find(mission_name.value);
      check(existing_mission == missions.end(), "La mission existe déjà.");

      missions.emplace(get_self(), [&](auto& new_mission) {
         new_mission.mission_name = mission_name;
         new_mission.target_attack_points = target_attack_points;
         new_mission.reward = reward;
         new_mission.is_completed = false;
         new_mission.total_attack_points = 0;
         new_mission.deadline = time_point_sec(mission_deadline); // Utilisez time_point_sec pour définir la date limite
      });

      print("Mission créée : ", mission_name, " - Points d'attaque cible : ", target_attack_points, " - Récompense : ", reward, " - Date limite : ", mission_deadline);
   }

   ACTION hardenmiss() {
      require_auth(get_self()); // Assurez-vous que seul le compte du contrat peut exécuter cette action

      auto current_time = current_time_point().sec_since_epoch();
      auto one_day = 24 * 60 * 60; // 24 heures en secondes
      double hardening_percentage = 0.05; // Pourcentage de durcissement, exemple 5%

      missions_table missions(get_self(), get_self().value);
      auto itr = missions.begin();
      while (itr != missions.end()) {
         if (current_time >= itr->last_hardening_time + one_day) {
               // Calcule le nouveau target_attack_points avec le pourcentage de durcissement
               uint64_t new_target = itr->target_attack_points * (1 + hardening_percentage);
               missions.modify(itr, get_self(), [&](auto& m) {
                  m.target_attack_points = new_target;
                  m.last_hardening_time = current_time; // Mise à jour du temps de dernier durcissement
               });
         }
         itr++;
      }
    }

   ACTION sendattack(name player, name mission_name) {
      require_auth(player);

      // Accéder à la table des missions
      missions_table missions(get_self(), get_self().value);
      auto existing_mission = missions.find(mission_name.value);
      check(existing_mission != missions.end(), "La mission n'existe pas.");
      check(!existing_mission->is_completed, "La mission est déjà terminée.");

      // Obtenir le temps actuel
      int64_t current_time_seconds = current_time_point().sec_since_epoch();

      // Accéder à la table des missions des joueurs avec l'index secondaire 'byplayer'
      player_missions_table player_missions(get_self(), get_self().value);
      auto player_index = player_missions.get_index<"byplayer"_n>();
      auto player_mission_itr = player_index.lower_bound(player.value);

      // Initialiser les variables
      uint64_t attack_points = 0;
      uint64_t move_cost = 0;
      bool is_in_forge = false;
      
      // Vérifier si le joueur est dans la forge
      forge_table forges(get_self(), get_self().value);
      auto existing_forge = forges.find(player.value);
      if(existing_forge != forges.end()) {
         is_in_forge = true;
      }

      // Vérifier si le joueur est un propriétaire
      owners_table owners(get_self(), get_self().value);
      auto existing_owner = owners.find(player.value);
      if (existing_owner != owners.end()) {
         attack_points = is_in_forge ? existing_owner->totalAttackArm : existing_owner->totalAttack;
         move_cost = existing_owner->totalMoveCost;
      } else {
         // Sinon, vérifier si le joueur est un joueur ordinaire
         players_table players(get_self(), get_self().value);
         auto existing_player = players.find(player.value);
         if (existing_player != players.end()) {
               attack_points = is_in_forge ? existing_player->totalAttackArm : existing_player->totalAttack;
               move_cost = existing_player->totalMoveCost;
         }
      }

      check(attack_points > 0, "Les points d'attaque doivent être supérieurs à zéro.");

      // Calculer les points d'attaque utiles
      uint64_t useful_attack_points = std::min(attack_points, existing_mission->target_attack_points - existing_mission->total_attack_points);

      // Calculer la période de refroidissement
      uint64_t cooldown_period = 24 * 3600 + (move_cost / 100);

      // Trouver ou créer l'entrée correspondante dans la table player_missions
      bool entry_found = false;
      for (; player_mission_itr != player_index.end() && player_mission_itr->player == player; ++player_mission_itr) {
         if (player_mission_itr->mission_name == mission_name) {
               entry_found = true;
               // Vérifier si le cooldown est respecté
               uint64_t time_since_last_attack = current_time_seconds - player_mission_itr->last_participation_time;
               uint64_t remaining_cooldown = cooldown_period > time_since_last_attack ? cooldown_period - time_since_last_attack : 0;

               check(time_since_last_attack >= cooldown_period, "Vous devez attendre " + std::to_string(remaining_cooldown) + " secondes avant de participer à nouveau.");

               // Mettre à jour l'entrée existante
               player_index.modify(player_mission_itr, get_self(), [&](auto& mod_player_mission) {
                  mod_player_mission.last_participation_time = current_time_seconds;
                  mod_player_mission.attack_points += useful_attack_points;
               });
               break;
         }
      }

      if (!entry_found) {
         // Créer une nouvelle entrée si le joueur n'a pas encore attaqué cette mission
         player_missions.emplace(get_self(), [&](auto& new_player_mission) {
               new_player_mission.id = player_missions.available_primary_key();
               new_player_mission.mission_name = mission_name;
               new_player_mission.player = player;
               new_player_mission.attack_points = useful_attack_points;
               new_player_mission.last_participation_time = current_time_seconds;
         });
      }


      // Mettre à jour les points d'attaque totaux pour la mission
      missions.modify(existing_mission, get_self(), [&](auto& mod_mission) {
         mod_mission.total_attack_points += useful_attack_points;
         if (mod_mission.total_attack_points >= mod_mission.target_attack_points) {
               mod_mission.is_completed = true;
               // Logique supplémentaire si la mission est complétée (par exemple, distribuer des récompenses)
         }
      });

      // Informer l'utilisateur de l'attaque réussie
      print("Attaque envoyée par le joueur ", player, " pour la mission ", mission_name);
   }

   ACTION distributere(name mission_name) {
      require_auth(get_self());

      missions_table missions(_self, _self.value);
      auto mission_itr = missions.find(mission_name.value);
      check(mission_itr != missions.end(), "Mission does not exist.");
      check(mission_itr->is_completed, "Mission is not yet completed.");
      check(!mission_itr->is_distributed, "Rewards have already been distributed for this mission.");
      check(mission_itr->total_attack_points > 0, "Total attack points must be positive.");

      player_missions_table player_missions(_self, _self.value);
      auto mission_index = player_missions.get_index<"bymission"_n>();
      auto lower = mission_index.lower_bound(mission_name.value);
      auto upper = mission_index.upper_bound(mission_name.value);
      int64_t rewards_distributed = 0;

      for (auto it = lower; it != upper; ++it) {
         check(it->attack_points > 0, "Player attack points must be positive.");
         double player_ratio = static_cast<double>(it->attack_points) / static_cast<double>(mission_itr->total_attack_points);
         asset player_reward = asset(int64_t(player_ratio * mission_itr->reward.amount), mission_itr->reward.symbol);

         check(player_reward.amount > 0, "Calculated player reward is not positive.");
         rewards_distributed += player_reward.amount;

         // Transfer the reward from the contract account to the player's account.
         action(
               permission_level{get_self(), "active"_n},
               "alien.worlds"_n, "transfer"_n,
               std::make_tuple(get_self(), it->player, player_reward, "Mission reward: " + mission_name.to_string())
         ).send();
      }

      // Update the mission to indicate that the rewards have been distributed.
      missions.modify(mission_itr, get_self(), [&](auto& mod_mission) {
         mod_mission.is_distributed = true;
      });
   }



   ACTION addforge(name player) {
    require_auth(get_self());

    // Vérifier si le joueur est déjà dans la forge
    forge_table forge(get_self(), get_self().value);
    auto itr = forge.find(player.value);
    check(itr == forge.end(), "Player already in forge");

    // Ajouter le joueur à la forge
    forge.emplace(get_self(), [&](auto& row) {
        row.player_address = player;
    });

    // Mettre à jour les soutiens
    upsupport(player);
}




[[eosio::on_notify("alien.worlds::transfer")]]

void on_transfer(name from, name to, asset quantity, std::string memo) {
   check(false, "ici");
   // Ensure the transfer is to this contract and not from this contract, and the memo is not to be ignored.
   if (to != get_self() || from == get_self() || memo == "ignore_memo") return;
   
   // Check if the currency is TLM and proceed only if it is.
   if (quantity.symbol == symbol("TLM", 4)) {
      if (quantity.amount == 10000 * 10000) { // Assuming TLM has 4 decimal places
         addtoforge(from); 
         return;
      } else if (quantity.amount == 90 * 10000) { // Assuming TLM has 4 decimal places
         check(false, "here");
         addmember(from);
         return;
      }

      // Extract land_id from the memo for chest updates.
      auto separator_pos = memo.find(":");
      if (separator_pos != std::string::npos) { // Correct memo format check
         // Convert the substring after the separator to uint64_t for land_id
         uint64_t land_id = std::stoull(memo.substr(separator_pos + 1));
         
         // Proceed if land_id is greater than 0
         if (land_id > 0){
            // Check for a new level based on the TLM amount sent with a correct memo format.
            auto new_level = get_new_level(quantity.amount);

            // If a new level is successfully determined, update the chest level.
            if (new_level.has_value()) {
                update_chest_level(land_id, *new_level);
            }
         }
      }
   }
}



    private:
 void upsupport(name entity) {
    // Accès aux tables
    players_table _players(get_self(), get_self().value);
    owners_table _owners(get_self(), get_self().value);
    supports_table _supports(get_self(), get_self().value);
    forge_table _forge(get_self(), get_self().value);

    name owner_key = entity; // Par défaut, considérez l'entité comme le propriétaire
    bool is_owner_in_forge = _forge.find(entity.value) != _forge.end();
    uint64_t number_of_lands = 1;

    // Initialiser les scores pour le recalcul
    uint64_t total_defense_score = 0;
    uint64_t total_attack_score = 0;
    uint64_t total_move_cost = 0;

    // Vérifier si l'entité est un joueur et trouver le propriétaire qu'il soutient
    auto player_itr = _players.find(entity.value);
    if (player_itr != _players.end()) {
        // Si c'est un joueur, déterminez le propriétaire qu'il soutient
        for (auto& support : _supports) {
            if (std::find(support.supporters.begin(), support.supporters.end(), entity) != support.supporters.end()) {
                owner_key = support.owner_address; // Le propriétaire que ce joueur soutient
                break;
            }
        }
    } else {
        // Vérifier directement si l'entité est un propriétaire
        auto owner_itr = _owners.find(entity.value);
        if (owner_itr != _owners.end()) {
            number_of_lands = std::max(owner_itr->numberofland, uint64_t(1)); // Assurer un minimum de 1
        } else {
            // Si l'entité n'est pas trouvée comme propriétaire, terminez la fonction
            return;
        }
    }

    // Inclure les scores du propriétaire s'il est dans la forge
    auto owner_itr = _owners.find(owner_key.value);
    if (owner_itr != _owners.end()) {
        is_owner_in_forge = _forge.find(owner_key.value) != _forge.end(); // Vérifiez à nouveau pour le propriétaire
        total_defense_score += is_owner_in_forge ? owner_itr->totalDefenseArm : owner_itr->totalDefense;
        total_attack_score += is_owner_in_forge ? owner_itr->totalAttackArm : owner_itr->totalAttack;
        total_move_cost += owner_itr->totalMoveCost; // Le coût de déplacement n'est pas divisé par le nombre de terrains
    }

    // Trouver la ligne correspondante dans la table support pour le propriétaire
    auto support_itr = _supports.find(owner_key.value);
    if (support_itr != _supports.end()) {
        // Recalculer les scores pour chaque joueur supporteur
        for (const auto& supporter : support_itr->supporters) {
            auto player_supporter_itr = _players.find(supporter.value);
            if (player_supporter_itr != _players.end()) {
                bool is_player_in_forge = _forge.find(supporter.value) != _forge.end();
                total_defense_score += (is_player_in_forge ? player_supporter_itr->totalDefenseArm : player_supporter_itr->totalDefense) / number_of_lands;
                total_attack_score += (is_player_in_forge ? player_supporter_itr->totalAttackArm : player_supporter_itr->totalAttack) / number_of_lands;
                total_move_cost += player_supporter_itr->totalMoveCost / number_of_lands; // Diviser le coût de déplacement peut ne pas être logique; ajustez si nécessaire
            }
        }

        // Mettre à jour la ligne dans la table support avec les scores recalculés
        _supports.modify(support_itr, get_self(), [&](auto& s) {
            s.total_defense_score = total_defense_score;
            s.total_attack_score = total_attack_score;
            s.totalMoveCost = total_move_cost;
        });
    } else {
        eosio::print("Aucune ligne de support trouvée pour ce propriétaire.");
    }
}




   // Determine the new chest level based on TLM amount
   std::optional<uint64_t> get_new_level(uint64_t amount) {
      uint64_t tlm_amount = amount / 10000;
      std::optional<uint64_t> new_level; // Declare the variable within function scope

      switch(tlm_amount) {
         case 100: new_level = 1; break;
         case 200: new_level = 2; break;
         case 300: new_level = 3; break;
         case 400: new_level = 4; break;
         case 500: new_level = 5; break;
         case 1000: new_level = 6; break;
         case 1500: new_level = 7; break;
         case 2000: new_level = 8; break;
         case 2500: new_level = 9; break;
         case 3000: new_level = 10; break;
         case 3500: new_level = 11; break;
         case 4000: new_level = 12; break;
         case 5000: new_level = 13; break;
         case 6000: new_level = 14; break;
         case 7500: new_level = 15; break;
         case 10000: new_level = 16; break;
         default: return std::nullopt; // No matching level found
      }
      
      return new_level; // Return the calculated level
   }


    // Private function to update chest level
     void update_chest_level(uint64_t land_id, std::optional<uint64_t> new_level) {
        chests_table chests(get_self(), get_self().value);
        auto itr = chests.find(land_id);
        check(itr != chests.end(), "Chest not found");

        chests.modify(itr, get_self(), [&](auto& chest) {
            if (new_level.has_value()) {
                chest.chest_level = new_level.value(); // Update the chest level
            }
        });
    }


   void addmember(name player) {
      member_table member(get_self(), get_self().value);
      auto itr = member.find(player.value);
      check(itr == member.end(), "Player already in member");

      member.emplace(get_self(), [&](auto& row) {
         row.player_name = player;
      });
   }

   void addtoforge(name player) {
      forge_table forge(get_self(), get_self().value);
      auto itr = forge.find(player.value);
      check(itr == forge.end(), "Player already in forge");

      forge.emplace(get_self(), [&](auto& row) {
         row.player_address = player;
      });

      upsupport(player); // Pas besoin de spécifier le type ici
   }



};
