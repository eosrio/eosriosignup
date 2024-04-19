#include "eosriosignup.hpp"

array<unsigned char,33> validate_key(string key_str) {

    eosio_assert(key_str.length() == 53, "PBK_N_53");

    string pubkey_prefix("EOS");

    auto result = mismatch(pubkey_prefix.begin(), pubkey_prefix.end(), key_str.begin());
    eosio_assert(result.first == pubkey_prefix.end(), "NON_EOS_PBK");

    auto base58substr = key_str.substr(pubkey_prefix.length());
    vector<unsigned char> vch;

    eosio_assert(decode_base58(base58substr, vch), "DEC_FAIL");
    eosio_assert(vch.size() == 37, "INV_KEY");

    array<unsigned char,33> pubkey_data;

    copy_n(vch.begin(), 33, pubkey_data.begin());

    checksum160 check_pubkey;
    ripemd160(reinterpret_cast<char *>(pubkey_data.data()), 33, &check_pubkey);
    eosio_assert(memcmp(&check_pubkey.hash, &vch.end()[-4], 4) == 0, "INV_KEY");

    return pubkey_data;
}

void eosriosignup::transfer(account_name from, account_name to, asset quantity, string memo) {
    if (from == _self || to != _self) {
        return;
    }

    eosio_assert(quantity.symbol == CORE_SYMBOL, "INV_TOK");
    eosio_assert(quantity.is_valid(), "INV_TOK_TR");
    eosio_assert(quantity.amount > 0, "Q_NEG");

    memo.erase(memo.begin(), find_if(memo.begin(), memo.end(), [](int ch) {
        return !isspace(ch);
    }));

    memo.erase(find_if(memo.rbegin(), memo.rend(), [](int ch) {
        return !isspace(ch);
    }).base(), memo.end());

    auto separator_pos = memo.find(' ');
    if (separator_pos == string::npos) {
        separator_pos = memo.find('-');
    }
    eosio_assert(separator_pos != string::npos, "SEP_FAIL");

    // get account 
    string account_name_str = memo.substr(0, separator_pos);
    eosio_assert(account_name_str.length() == 12, "ACC_SIZE_ERR");
    account_name new_account_name = string_to_name(account_name_str.c_str());

    string keys_str = memo.substr(separator_pos + 1);

    separator_pos = keys_str.find(' ');
    if (separator_pos == string::npos) {
        separator_pos = keys_str.find('-');
    }
    eosio_assert(separator_pos != string::npos, "SEP_FAIL");

    string owner_key_str = keys_str.substr(0,separator_pos);
    string active_key_str;

    if ((separator_pos + 1) == string::npos){
        active_key_str = owner_key_str;
    }else{
        active_key_str = keys_str.substr(separator_pos + 1);
    }

    string pubkey_prefix("EOS");

    auto pubkey_data_o = validate_key(owner_key_str);
    auto pubkey_data_a = validate_key(active_key_str);

    // calculate current ram price
    rammarket_t _db(N(eosio),N(eosio));
    auto entry = _db.get(S(4,RAMCORE));
    double quote = entry.quote.balance.amount;
    double base = entry.base.balance.amount;
    double ram_price = quote / base;

    // Define account resources
    asset stake_net(2500, CORE_SYMBOL);
    asset stake_cpu(7500, CORE_SYMBOL);
    asset buy_ram(ram_price * 4096, CORE_SYMBOL);

    asset resources_value = stake_net + stake_cpu + buy_ram;
    asset transfer_value = quantity - resources_value;

    eosio_assert(transfer_value.amount >= 0, "LOW_BAL");

    // owner
    signup_public_key pubkey_o = {
        .type = 0,
        .data = pubkey_data_o,
    };
    key_weight pubkey_weight_o = {
        .key = pubkey_o,
        .weight = 1,
    };
    authority owner = authority{
        .threshold = 1,
        .keys = {pubkey_weight_o},
        .accounts = {},
        .waits = {}
    };

    // active
    signup_public_key pubkey_a = {
        .type = 0,
        .data = pubkey_data_a,
    };
    key_weight pubkey_weight_a = {
        .key = pubkey_a,
        .weight = 1,
    };
    authority active = authority{
        .threshold = 1,
        .keys = {pubkey_weight_a},
        .accounts = {},
        .waits = {}
    };
    newaccount new_account = newaccount{
        .creator = _self,
        .name = new_account_name,
        .owner = owner,
        .active = active
    };

    string new_memo = "initial transfer";

    action(
        permission_level{ _self, N(active) },
        N(eosio),
        N(newaccount),
        new_account
        ).send();

    action(
        permission_level{ _self, N(active)},
        N(eosio),
        N(buyram),
        make_tuple(_self, new_account_name, buy_ram)
        ).send();

    action(
        permission_level{ _self, N(active)},
        N(eosio),
        N(delegatebw),
        make_tuple(_self, new_account_name, stake_net, stake_cpu, true)
        ).send();

    action(
        permission_level{ _self, N(active)},
        N(eosio.token),
        N(transfer),
        make_tuple(_self, new_account_name, transfer_value, new_memo)
        ).send();

}
