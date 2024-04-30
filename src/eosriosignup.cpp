#include <eosriosignup.hpp>
#include <eosio/print.hpp>
#include <string>
#include <vector>

void eosriosignup::process_transfer(name from, name to, asset quantity, std::string memo)
{
    if (from == _self || to != _self)
    {
        return;
    }

    rammarket_t ram_db(name("eosio"), name("eosio").value);
    symbol ram_core = symbol(symbol_code("RAMCORE"), 4);
    auto entry = ram_db.get(ram_core.raw());

    // get the core symbol (EOS / WAX /...)
    symbol CORE_SYMBOL = entry.quote.balance.symbol;

    // calculate current ram price
    double quote = entry.quote.balance.amount;
    double base = entry.base.balance.amount;
    double ram_price = quote / base;

    check(quantity.symbol == CORE_SYMBOL, "INV_TOK");
    check(quantity.is_valid(), "INV_TOK_TR");
    check(quantity.amount > 0, "Q_NEG");

    // memo.erase(memo.begin(), find_if(memo.begin(), memo.end(), [](int ch)
    //                                  { return !isspace(ch); }));

    // memo.erase(find_if(memo.rbegin(), memo.rend(), [](int ch)
    //                    { return !isspace(ch); })
    //                .base(),
    //            memo.end());

    print_f("\nMemo: %\n", memo);

    std::vector<std::string> result;
    size_t pos = 0;
    std::string token;

    while ((pos = memo.find('-')) != std::string::npos)
    {
        token = memo.substr(0, pos);
        result.push_back(token);
        memo.erase(0, pos + 1); // +1 to skip the '-'
    }
    result.push_back(memo); // Add the last part

    // Print the split parts
    for (const auto &part : result)
    {
        print_f("\nPART: %", part);
    }

    // get account
    std::string account_name_str = result[0];
    check(account_name_str.length() == 12, "ACC_SIZE_ERR");
    name new_account_name = name(account_name_str.c_str());

    std::string owner_key_str;
    std::string active_key_str;

        if (result.size() == 3)
    {
        owner_key_str = result[1];
        active_key_str = result[2];
    }
    else if (result.size() == 2)
    {
        owner_key_str = result[1];
        active_key_str = result[1];
    }
    else
    {
        check(false, "WRONG_ARG_SIZE");
    }

    std::string pubkey_prefix("EOS");

    std::array<unsigned char, 33> pubkey_data_o = eosriosignup::validate_key(owner_key_str);
    std::array<unsigned char, 33> pubkey_data_a = eosriosignup::validate_key(active_key_str);

    // Define account resources
    asset stake_net(2500, CORE_SYMBOL);
    asset stake_cpu(7500, CORE_SYMBOL);
    asset buy_ram(ram_price * 4096, CORE_SYMBOL);

    asset resources_value = stake_net + stake_cpu + buy_ram;
    asset transfer_value = quantity - resources_value;

    check(transfer_value.amount >= 0, "LOW_BAL");

    // owner
    signup_public_key pubkey_o = {
        .type = 0,
        .data = pubkey_data_o,
    };

    key_weight pubkey_weight_o = {
        .key = pubkey_o,
        .weight = 1,
    };

    authority owner = {
        .threshold = 1,
        .keys = {pubkey_weight_o},
        .accounts = {},
        .waits = {}};

    // active
    signup_public_key pubkey_a = {
        .type = 0,
        .data = pubkey_data_a,
    };

    key_weight pubkey_weight_a = {
        .key = pubkey_a,
        .weight = 1,
    };

    authority active = {
        .threshold = 1,
        .keys = {pubkey_weight_a},
        .accounts = {},
        .waits = {}};

    newaccount new_account = {
        .creator = _self,
        .name = new_account_name,
        .owner = owner,
        .active = active};

    std::string new_memo = "initial transfer";

    permission_level self_active = {get_self(), name("active")};
    name eosio_name = name("eosio");

    // Create new account
    action(self_active, eosio_name, name("newaccount"), new_account).send();

    // Buy RAM
    std::tuple buyram_data = std::make_tuple(_self, new_account_name, buy_ram);
    action(self_active, eosio_name, name("buyram"), std::make_tuple(_self, new_account_name, buy_ram)).send();

    // Delegate CPU and NET
    std::tuple dbw_data = std::make_tuple(_self, new_account_name, stake_net, stake_cpu, true);
    action(self_active, eosio_name, name("delegatebw"), dbw_data).send();

    // Transfer remaining balance to new account
    std::tuple transfer_data = std::make_tuple(_self, new_account_name, transfer_value, new_memo);
    action(self_active, name("eosio.token"), name("transfer"), transfer_data).send();
}