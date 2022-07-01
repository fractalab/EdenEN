#pragma once

#include <constants.hpp>
#include <eden-atomicassets.hpp>
#include <eden_dispatcher.hpp>
#include <encrypt.hpp>
#include <eosio/asset.hpp>
#include <eosio/bytes.hpp>
#include <eosio/eosio.hpp>
#include <inductions.hpp>
#include <sessions.hpp>
#include <string>
#include <vector>

#ifdef ENABLE_SET_TABLE_ROWS
#include <accounts.hpp>
#include <auctions.hpp>
#include <boost/mp11/list.hpp>
#include <bylaws.hpp>
#include <distributions.hpp>
#include <elections.hpp>
#include <globals.hpp>
#include <inductions.hpp>
#include <members.hpp>
#include <migrations.hpp>
#endif

namespace eden
{
   // Ricardian contracts live in eden-ricardian.cpp
   extern const char* withdraw_ricardian;
   extern const char* genesis_ricardian;
   extern const char* clearall_ricardian;
   extern const char* inductinit_ricardian;
   extern const char* inductprofil_ricardian;
   extern const char* inductvideo_ricardian;
   extern const char* inductendors_ricardian;
   extern const char* inductdonate_ricardian;
   extern const char* inductcancel_ricardian;
   extern const char* gc_ricardian;
   extern const char* inducted_ricardian;
   extern const char* peacetreaty_clause;
   extern const char* bylaws_clause;

   // Placeholder; the ABI generator redefines this
   using verb = std::variant<int>;

#ifdef ENABLE_SET_TABLE_ROWS
   using table_variant = boost::mp11::mp_append<account_variant,
                                                auction_variant,
                                                bylaws_variant,
                                                distribution_account_variant,
                                                distribution_variant,
                                                endorsement_variant,
                                                current_election_state,
                                                election_state_variant,
                                                global_variant,
                                                induction_variant,
                                                member_variant,
                                                member_stats_variant,
                                                migration_variant,
                                                pool_variant,
                                                encrypted_data_variant,
                                                std::variant<vote>>;
#endif

   class eden : public eosio::contract
   {
     public:
      using contract::contract;

      eden(eosio::name receiver, eosio::name code, eosio::datastream<const char*> ds)
          : contract(receiver, code, ds)
      {
      }

      void notify_transfer(eosio::name from,
                           eosio::name to,
                           const eosio::asset& quantity,
                           std::string memo);

      void newsession(eosio::name eden_account,
                      const eosio::public_key& key,
                      eosio::block_timestamp expiration,
                      const std::string& description);

      void delsession(const eosio::not_in_abi<session_info>& current_session,
                      eosio::name eden_account,
                      const eosio::public_key& key);

      void run(eosio::ignore<run_auth> auth, eosio::ignore<std::vector<verb>> verbs);

      void withdraw(eosio::name owner, const eosio::asset& quantity);

      void donate(eosio::name payer, const eosio::asset& quantity);

      void genesis(std::string community,
                   eosio::symbol community_symbol,
                   eosio::asset minimum_donation,
                   std::vector<eosio::name> initial_members,
                   std::string genesis_video,
                   atomicassets::attribute_map collection_attributes,
                   eosio::asset auction_starting_bid,
                   uint32_t auction_duration,
                   const std::string& memo,
                   uint8_t election_day,
                   const std::string& election_time);

      void modifymindon(eosio::asset minimum_donation);
      
      void addtogenesis(eosio::name new_genesis_member, eosio::time_point expiration);
      void gensetexpire(uint64_t induction_id, eosio::time_point new_expiration);

      void clearall();

      void inductinit(const eosio::not_in_abi<session_info>& current_session,
                      uint64_t id,
                      eosio::name inviter,
                      eosio::name invitee,
                      std::vector<eosio::name> witnesses);

      void inductprofil(const eosio::not_in_abi<session_info>& current_session,
                        uint64_t id,
                        new_member_profile new_member_profile);

      void inductvideo(const eosio::not_in_abi<session_info>& current_session,
                       eosio::name account,
                       uint64_t id,
                       std::string video);

      void inductendors(const eosio::not_in_abi<session_info>& current_session,
                        eosio::name account,
                        uint64_t id,
                        eosio::checksum256 induction_data_hash);

      void inductdonate(eosio::name payer, uint64_t id, const eosio::asset& quantity);

      void inductcancel(const eosio::not_in_abi<session_info>& current_session,
                        eosio::name account,
                        uint64_t id);
      void inductmeetin(const eosio::not_in_abi<session_info>& current_session,
                        eosio::name account,
                        uint64_t id,
                        const std::vector<encrypted_key>& keys,
                        const eosio::bytes& data,
                        const std::optional<eosio::bytes>& old_data);

      void inducted(eosio::name inductee);

      void resign(eosio::name member);

      void rename(eosio::name member, eosio::name newaccount);

      void setencpubkey(eosio::name member, const eosio::public_key& key);

      void electsettime(eosio::time_point_sec election_time);

      void electconfig(uint8_t election_day,
                       const std::string& election_time,
                       uint32_t round_duration_sec);

      void electopt(const eosio::not_in_abi<session_info>& current_session,
                    eosio::name member,
                    bool participating);

      void electseed(const eosio::bytes& btc_header);
      void electmeeting(const eosio::not_in_abi<session_info>& current_session,
                        eosio::name account,
                        uint8_t round,
                        const std::vector<encrypted_key>& keys,
                        const eosio::bytes& data,
                        const std::optional<eosio::bytes>& old_data);
      void electvote(const eosio::not_in_abi<session_info>& current_session,
                     uint8_t round,
                     eosio::name voter,
                     eosio::name candidate);
      void electvideo(const eosio::not_in_abi<session_info>& current_session,
                      uint8_t round,
                      eosio::name voter,
                      const std::string& video);
      void electprocess(uint32_t max_steps);

      void distribute(uint32_t max_steps);

      void fundtransfer(eosio::name from,
                        eosio::block_timestamp distribution_time,
                        uint8_t rank,
                        eosio::name to,
                        eosio::asset amount,
                        const std::string& memo);
      void usertransfer(eosio::name from,
                        eosio::name to,
                        eosio::asset amount,
                        const std::string& memo);

      void bylawspropose(eosio::name proposer, const std::string& bylaws);
      void bylawsapprove(eosio::name approver, const eosio::checksum256& bylaws_hash);
      void bylawsratify(eosio::name approver, const eosio::checksum256& bylaws_hash);

      void gc(uint32_t limit);

      // Update contract tables to the latest version of the contract, where necessary.
      // New functionality may be unavailable until this is complete.
      void migrate(uint32_t limit);
      // For testing only.
      void unmigrate();

#ifdef ENABLE_SET_TABLE_ROWS
      void settablerows(eosio::name scope, const std::vector<table_variant>&);
#endif

      void notify_lognewtempl(int32_t template_id,
                              eosio::name authorized_creator,
                              eosio::name collection_name,
                              eosio::name schema_name,
                              bool transferable,
                              bool burnable,
                              uint32_t max_supply,
                              const atomicassets::attribute_map& immutable_data);

      void notify_logmint(uint64_t asset_id,
                          eosio::name authorized_minter,
                          eosio::name collection_name,
                          eosio::name schema_name,
                          int32_t template_id,
                          eosio::name new_asset_owner,
                          eosio::ignore<atomicassets::attribute_map>,
                          eosio::ignore<atomicassets::attribute_map>,
                          eosio::ignore<std::vector<eosio::asset>>);
   };

   EDEN_ACTIONS(
       eden,
       "eden.gm"_n,
       // action(newsession, eden_account, key, expiration, description),
       // eden_verb(delsession, 0, eden_account, key),
       // action(run, auth, verbs),
       action(withdraw, owner, quantity, ricardian_contract(withdraw_ricardian)),
       action(donate, owner, quantity),
       action(fundtransfer, from, distribution_time, rank, to, amount, memo),
       action(usertransfer, from, to, amount, memo),
       action(genesis,
              community,
              community_symbol,
              minimum_donation,
              initial_members,
              genesis_video,
              collection_attributes,
              auction_starting_bid,
              auction_duration,
              memo,
              election_day,
              election_time,
              ricardian_contract(genesis_ricardian)),
       action(modifymindon, minimum_donation),
       action(addtogenesis, account, expiration),
       action(gensetexpire, id, new_expiration),
       action(clearall, ricardian_contract(clearall_ricardian)),
       eden_verb(inductinit,
                 10,
                 id,
                 inviter,
                 invitee,
                 witnesses,
                 ricardian_contract(inductinit_ricardian)),
       eden_verb(inductmeetin, 1, account, id, keys, data, old_data),
       eden_verb(inductprofil,
                 2,
                 id,
                 new_member_profile,
                 ricardian_contract(inductprofil_ricardian)),
       eden_verb(inductvideo, 3, account, id, video, ricardian_contract(inductvideo_ricardian)),
       eden_verb(inductendors,
                 4,
                 account,
                 id,
                 induction_data_hash,
                 ricardian_contract(inductendors_ricardian)),
       action(setencpubkey, account, key),
       action(electsettime, election_time),
       action(electconfig, day, time, round_duration),
       eden_verb(electopt, 5, member, participating),
       action(electseed, btc_header),
       eden_verb(electmeeting, 6, account, round, keys, data, old_data),
       eden_verb(electvote, 7, round, voter, candidate),
       eden_verb(electvideo, 8, round, voter, video),
       action(electprocess, max_steps),
       action(bylawspropose, proposer, bylaws),
       action(bylawsapprove, approver, bylaws_hash),
       action(bylawsratify, approver, bylaws_hash),
       action(distribute, max_steps),
       action(inductdonate, payer, id, quantity, ricardian_contract(inductdonate_ricardian)),
       eden_verb(inductcancel, 9, account, id, ricardian_contract(inductcancel_ricardian)),
       action(inducted, inductee, ricardian_contract(inducted_ricardian)),
       action(resign, account),
       action(rename, old_account, new_account),
       action(gc, limit, ricardian_contract(gc_ricardian)),
       action(migrate, limit),
       action(unmigrate),
#ifdef ENABLE_SET_TABLE_ROWS
       action(settablerows, scope, rows),
#endif
       notify(token_contract, transfer),
       notify(atomic_assets_account, lognewtempl),
       notify(atomic_assets_account, logmint))
}  // namespace eden
