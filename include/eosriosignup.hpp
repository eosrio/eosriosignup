#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/action.hpp>
#include <eosio/symbol.hpp>
#include <eosio/crypto.hpp>
#include <eosio/contract.hpp>
#include <eosio/print.hpp>
#include <cstring>
#include <constants.hpp>

using namespace eosio;

CONTRACT eosriosignup : public contract
{
public:
   using contract::contract;

   [[eosio::on_notify("eosio.token::transfer")]] void process_transfer(name from, name to, asset quantity, std::string memo);

private:
   struct signup_public_key
   {
      uint8_t type;
      std::array<unsigned char, 33> data;
   };

   struct permission_level_weight
   {
      permission_level permission;
      uint16_t weight;
   };

   struct key_weight
   {
      signup_public_key key;
      uint16_t weight;
   };

   struct wait_weight
   {
      uint32_t wait_sec;
      uint16_t weight;
   };

   struct authority
   {
      uint32_t threshold;
      std::vector<key_weight> keys;
      std::vector<permission_level_weight> accounts;
      std::vector<wait_weight> waits;
   };

   struct newaccount
   {
      name creator;
      name name;
      authority owner;
      authority active;
   };

   // RAM market - From: https://github.com/AntelopeIO/reference-contracts/blob/main/contracts/eosio.system/include/eosio.system/exchange_state.hpp
   struct exchange_state
   {
      asset supply;

      struct connector
      {
         asset balance;
         double weight;
         EOSLIB_SERIALIZE(connector, (balance)(weight))
      };

      connector base;
      connector quote;
      uint64_t primary_key() const { return supply.symbol.raw(); }
      EOSLIB_SERIALIZE(exchange_state, (supply)(base)(quote))
   };

   typedef multi_index<"rammarket"_n, exchange_state> rammarket_t;

   std::array<unsigned char, 33> validate_key(std::string key_str)
   {

      std::string pubkey_prefix;
      std::string base58substr;
      std::vector<unsigned char> vch;

      print_f("\n Key Length: %", key_str.length());

      if (key_str.length() == 53)
      {
         pubkey_prefix = "EOS";
         auto result = mismatch(pubkey_prefix.begin(), pubkey_prefix.end(), key_str.begin());
         check(result.first == pubkey_prefix.end(), "NON_EOS_PBK");
         base58substr = key_str.substr(pubkey_prefix.length());
      }
      else if (key_str.length() == 57)
      {
         pubkey_prefix = "PUB_";
         auto result = mismatch(pubkey_prefix.begin(), pubkey_prefix.end(), key_str.begin());
         check(result.first == pubkey_prefix.end(), "NON_PUB_PBK");
         base58substr = key_str.substr(pubkey_prefix.length() + 3);
      }

      check(key_str.length() == 53 || key_str.length() == 57, "PBK_SIZE_ERR");

      check(decode_base58(base58substr, vch), "DEC_FAIL");
      check(vch.size() == 37, "INV_KEY");

      std::array<unsigned char, 33> pubkey_data;

      copy_n(vch.begin(), 33, pubkey_data.begin());

      // checksum160 check_pubkey;
      // ripemd160(reinterpret_cast<char *>(pubkey_data.data()), 33, &check_pubkey);
      // check(memcmp(&check_pubkey.hash, &vch.end()[-4], 4) == 0, "INV_KEY");

      return pubkey_data;
   }

   bool DecodeBase58(const char *psz, std::vector<unsigned char> &vch)
   {
      // Skip leading spaces.
      while (*psz && isspace(*psz))
         psz++;
      // Skip and count leading '1's.
      int zeroes = 0;
      int length = 0;
      while (*psz == '1')
      {
         zeroes++;
         psz++;
      }
      // Allocate enough space in big-endian base256 representation.
      int size = strlen(psz) * 733 / 1000 + 1; // log(58) / log(256), rounded up.
      std::vector<unsigned char> b256(size);
      // Process the characters.
      static_assert(sizeof(mapBase58) / sizeof(mapBase58[0]) == 256, "mapBase58.size() should be 256"); // guarantee not out of range
      while (*psz && !isspace(*psz))
      {
         // Decode base58 character
         int carry = mapBase58[(uint8_t)*psz];
         if (carry == -1) // Invalid b58 character
            return false;
         int i = 0;
         for (std::vector<unsigned char>::reverse_iterator it = b256.rbegin(); (carry != 0 || i < length) && (it != b256.rend()); ++it, ++i)
         {
            carry += 58 * (*it);
            *it = carry % 256;
            carry /= 256;
         }
         check(carry == 0, "Non-zero carry");
         length = i;
         psz++;
      }
      // Skip trailing spaces.
      while (isspace(*psz))
         psz++;
      if (*psz != 0)
         return false;
      // Skip leading zeroes in b256.
      std::vector<unsigned char>::iterator it = b256.begin() + (size - length);
      while (it != b256.end() && *it == 0)
         it++;
      // Copy result into output vector.
      vch.reserve(zeroes + (b256.end() - it));
      vch.assign(zeroes, 0x00);
      while (it != b256.end())
         vch.push_back(*(it++));
      return true;
   }

   bool decode_base58(const std::string &str, std::vector<unsigned char> &vch)
   {
      return DecodeBase58(str.c_str(), vch);
   }
};
