#include <eosriosignup.hpp>

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

    memo.erase(memo.begin(), find_if(memo.begin(), memo.end(), [](int ch)
                                     { return !isspace(ch); }));

    memo.erase(find_if(memo.rbegin(), memo.rend(), [](int ch)
                       { return !isspace(ch); })
                   .base(),
               memo.end());

    auto separator_pos = memo.find(' ');
    if (separator_pos == std::string::npos)
    {
        separator_pos = memo.find('-');
    }
    check(separator_pos != std::string::npos, "SEP_FAIL");

    // get account
    std::string account_name_str = memo.substr(0, separator_pos);
    check(account_name_str.length() == 12, "ACC_SIZE_ERR");
    name new_account_name = name(account_name_str.c_str());

    std::string keys_str = memo.substr(separator_pos + 1);

    separator_pos = keys_str.find(' ');
    if (separator_pos == std::string::npos)
    {
        separator_pos = keys_str.find('-');
    }
    check(separator_pos != std::string::npos, "SEP_FAIL");

    std::string owner_key_str = keys_str.substr(0, separator_pos);
    std::string active_key_str;

    if ((separator_pos + 1) == std::string::npos)
    {
        active_key_str = owner_key_str;
    }
    else
    {
        active_key_str = keys_str.substr(separator_pos + 1);
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