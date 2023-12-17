#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>

using namespace eosio;

CONTRACT mycontract : public contract {
public:
    using contract::contract;

    // Table pour stocker les informations des owners
    TABLE owner_struct {
        name owner_address;
        uint64_t defense_score;

        auto primary_key() const { return owner_address.value; }
    };

    // Table pour stocker les informations des players
    TABLE player_struct {
        name player_address;
        uint64_t defense_score;

        auto primary_key() const { return player_address.value; }
    };

    // Structure pour enregistrer les informations de soutien
    TABLE support_struct {
        name owner_address;
        std::vector<name> supporters; // Liste des players qui soutiennent cet owner
        uint64_t total_defense_score; // Score de défense total (owner + supporters)

        auto primary_key() const { return owner_address.value; }
    };

    typedef multi_index<"owners"_n, owner_struct> owners_table;
    typedef multi_index<"players"_n, player_struct> players_table;
    typedef multi_index<"supports"_n, support_struct> supports_table;

    ACTION addowner(name owner, uint64_t defense_score) {
        require_auth(get_self());

        owners_table owners(get_self(), get_self().value);
        auto it = owners.find(owner.value);
        check(it == owners.end(), "Owner already exists");

        owners.emplace(get_self(), [&](auto& row) {
            row.owner_address = owner;
            row.defense_score = defense_score; // Ajouter le score de défense
        });
    }
    // Action pour supprimer un élément de la table
    ACTION removeowner(name owner_address) {
        require_auth(get_self());

        owners_table owners(get_self(), get_self().value);

        auto iterator = owners.find(owner_address.value);
        check(iterator != owners.end(), "Owner not found");

        owners.erase(iterator);
    }


    ACTION addplayer(name player, uint64_t defense_score) {
        require_auth(get_self());

        players_table players(get_self(), get_self().value);
        auto it = players.find(player.value);
        check(it == players.end(), "Player already exists");

        players.emplace(get_self(), [&](auto& row) {
            row.player_address = player;
            row.defense_score = defense_score; // Ajouter le score de défense
        });
    }



    [[eosio::on_notify("eosio.token::transfer")]]
    void on_transfer(name from, name to, asset quantity, std::string memo) {
        if (to == get_self() && quantity.symbol == symbol("TLM", 4)) {
            check(quantity.amount == 1000000 , "Incorrect amount. The registration fee is 10 TLM.");

            players_table players(get_self(), get_self().value);
            players.emplace(get_self(), [&](auto& row) {
                row.player_address = from;
            });
        }
    }
};



