#include <boost/multi_index/key.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>
#include <chainbase/chainbase.hpp>
#include <clchain/crypto.hpp>
#include <clchain/graphql_connection.hpp>
#include <clchain/subchain.hpp>
#include <eden.hpp>
#include <eosio/from_bin.hpp>
#include <eosio/to_bin.hpp>

using namespace eosio::literals;

eosio::name eden_account;
eosio::name token_account;
eosio::name atomic_account;
eosio::name atomicmarket_account;

// TODO: switch to uint64_t (js BigInt) after we upgrade to nodejs >= 15
extern "C" void __wasm_call_ctors();
[[clang::export_name("initialize")]] void initialize(uint32_t eden_account_low,
                                                     uint32_t eden_account_high,
                                                     uint32_t token_account_low,
                                                     uint32_t token_account_high,
                                                     uint32_t atomic_account_low,
                                                     uint32_t atomic_account_high,
                                                     uint32_t atomicmarket_account_low,
                                                     uint32_t atomicmarket_account_high)
{
   __wasm_call_ctors();
   eden_account.value = (uint64_t(eden_account_high) << 32) | eden_account_low;
   token_account.value = (uint64_t(token_account_high) << 32) | token_account_low;
   atomic_account.value = (uint64_t(atomic_account_high) << 32) | atomic_account_low;
   atomicmarket_account.value =
       (uint64_t(atomicmarket_account_high) << 32) | atomicmarket_account_low;
}

[[clang::export_name("allocateMemory")]] void* allocateMemory(uint32_t size)
{
   return malloc(size);
}
[[clang::export_name("freeMemory")]] void freeMemory(void* p)
{
   free(p);
}

std::variant<std::string, std::vector<char>> result;
[[clang::export_name("getResultSize")]] uint32_t getResultSize()
{
   return std::visit([](auto& data) { return data.size(); }, result);
}
[[clang::export_name("getResult")]] const char* getResult()
{
   return std::visit([](auto& data) { return data.data(); }, result);
}

template <typename T>
void dump(const T& ind)
{
   printf("%s\n", eosio::format_json(ind).c_str());
}

namespace boost
{
   BOOST_NORETURN void throw_exception(std::exception const& e)
   {
      eosio::detail::assert_or_throw(e.what());
   }
   BOOST_NORETURN void throw_exception(std::exception const& e, boost::source_location const& loc)
   {
      eosio::detail::assert_or_throw(e.what());
   }
}  // namespace boost

struct by_id;
struct by_pk;
struct by_invitee;

template <typename T, typename... Indexes>
using mic = boost::
    multi_index_container<T, boost::multi_index::indexed_by<Indexes...>, chainbase::allocator<T>>;

template <typename T>
using ordered_by_id = boost::multi_index::ordered_unique<  //
    boost::multi_index::tag<by_id>,
    boost::multi_index::key<&T::id>>;

template <typename T>
using ordered_by_pk = boost::multi_index::ordered_unique<  //
    boost::multi_index::tag<by_pk>,
    boost::multi_index::key<&T::pk>>;

template <typename T>
using ordered_by_invitee = boost::multi_index::ordered_unique<  //
    boost::multi_index::tag<by_invitee>,
    boost::multi_index::key<&T::invitee>>;

uint64_t available_pk(const auto& table, const auto& first)
{
   auto& idx = table.template get<by_pk>();
   if (idx.empty())
      return first;
   return (--idx.end())->pk() + 1;
}

enum tables
{
   status_table,
   induction_table,
   member_table,
};

struct status
{
   bool active = false;
   std::string community;
   eosio::symbol communitySymbol;
   eosio::asset minimumDonation;
   std::vector<eosio::name> initialMembers;
   std::string genesisVideo;
   eden::atomicassets::attribute_map collectionAttributes;
   eosio::asset auctionStartingBid;
   uint32_t auctionDuration;
   std::string memo;
};
EOSIO_REFLECT(status,
              active,
              community,
              communitySymbol,
              minimumDonation,
              initialMembers,
              genesisVideo,
              collectionAttributes,
              auctionStartingBid,
              auctionDuration,
              memo)

struct status_object : public chainbase::object<status_table, status_object>
{
   CHAINBASE_DEFAULT_CONSTRUCTOR(status_object)

   id_type id;
   status status;
};
using status_index = mic<status_object, ordered_by_id<status_object>>;

struct induction
{
   uint64_t id = 0;
   eosio::name inviter;
   eosio::name invitee;
   std::vector<eosio::name> witnesses;
   eden::new_member_profile profile;
   std::string video;
};
EOSIO_REFLECT(induction, id, inviter, invitee, witnesses, profile, video)

struct induction_object : public chainbase::object<induction_table, induction_object>
{
   CHAINBASE_DEFAULT_CONSTRUCTOR(induction_object)

   id_type id;
   induction induction;

   uint64_t pk() const { return induction.id; }
   std::pair<eosio::name, uint64_t> invitee() const { return {induction.invitee, induction.id}; }
};
using induction_index = mic<induction_object,
                            ordered_by_id<induction_object>,
                            ordered_by_pk<induction_object>,
                            ordered_by_invitee<induction_object>>;

struct member
{
   eosio::name account;
   eosio::name inviter;
   std::vector<eosio::name> inductionWitnesses;
   eden::new_member_profile profile;
   std::string inductionVideo;
};
EOSIO_REFLECT(member, account, inviter, inductionWitnesses, profile, inductionVideo)

struct member_object : public chainbase::object<member_table, member_object>
{
   CHAINBASE_DEFAULT_CONSTRUCTOR(member_object)

   id_type id;
   member member;

   eosio::name pk() const { return member.account; }
};
using member_index = mic<member_object, ordered_by_id<member_object>, ordered_by_pk<member_object>>;

struct database
{
   chainbase::database db;
   chainbase::generic_index<status_index> status;
   chainbase::generic_index<induction_index> inductions;
   chainbase::generic_index<member_index> members;

   database()
   {
      db.add_index(status);
      db.add_index(inductions);
      db.add_index(members);
   }
};
database db;

template <typename Tag, typename Table, typename Key, typename F>
void add_or_modify(Table& table, const Key& key, F&& f)
{
   auto& idx = table.template get<Tag>();
   auto it = idx.find(key);
   if (it != idx.end())
      table.modify(*it, [&](auto& obj) { return f(false, obj); });
   else
      table.emplace([&](auto& obj) { return f(true, obj); });
}

template <typename Tag, typename Table, typename Key, typename F>
void add_or_replace(Table& table, const Key& key, F&& f)
{
   auto& idx = table.template get<Tag>();
   auto it = idx.find(key);
   if (it != idx.end())
      table.remove(*it);
   table.emplace(f);
}

template <typename Tag, typename Table, typename Key, typename F>
void modify(Table& table, const Key& key, F&& f)
{
   auto& idx = table.template get<Tag>();
   auto it = idx.find(key);
   eosio::check(it != idx.end(), "missing record");
   table.modify(*it, [&](auto& obj) { return f(obj); });
}

template <typename Tag, typename Table, typename Key>
void remove_if_exists(Table& table, const Key& key)
{
   auto& idx = table.template get<Tag>();
   auto it = idx.find(key);
   if (it != idx.end())
      table.remove(*it);
}

template <typename Tag, typename Table, typename Key>
const auto& get(Table& table, const Key& key)
{
   auto& idx = table.template get<Tag>();
   auto it = idx.find(key);
   eosio::check(it != idx.end(), "missing record");
   return *it;
}

const auto& get_status()
{
   auto& idx = db.status.get<by_id>();
   eosio::check(idx.size() == 1, "missing genesis action");
   return *idx.begin();
}

void add_genesis_member(const status& status, eosio::name member)
{
   db.inductions.emplace([&](auto& obj) {
      obj.induction.id = available_pk(db.inductions, 1);
      obj.induction.inviter = eden_account;
      obj.induction.invitee = member;
      for (auto witness : status.initialMembers)
         if (witness != member)
            obj.induction.witnesses.push_back(witness);
   });
}

void clearall(auto& table)
{
   for (auto it = table.begin(); it != table.end();)
   {
      auto next = it;
      ++next;
      table.remove(*it);
      it = next;
   }
}

void clearall()
{
   clearall(db.status);
   clearall(db.inductions);
   clearall(db.members);
}

void genesis(std::string community,
             eosio::symbol community_symbol,
             eosio::asset minimum_donation,
             std::vector<eosio::name> initial_members,
             std::string genesis_video,
             eden::atomicassets::attribute_map collection_attributes,
             eosio::asset auction_starting_bid,
             uint32_t auction_duration,
             std::string memo)
{
   auto& idx = db.status.get<by_id>();
   eosio::check(idx.empty(), "duplicate genesis action");
   db.status.emplace([&](auto& obj) {
      obj.status.community = std::move(community);
      obj.status.communitySymbol = std::move(community_symbol);
      obj.status.minimumDonation = std::move(minimum_donation);
      obj.status.initialMembers = std::move(initial_members);
      obj.status.genesisVideo = std::move(genesis_video);
      obj.status.collectionAttributes = std::move(collection_attributes);
      obj.status.auctionStartingBid = std::move(auction_starting_bid);
      obj.status.auctionDuration = std::move(auction_duration);
      obj.status.memo = std::move(memo);
      for (auto& member : obj.status.initialMembers)
         add_genesis_member(obj.status, member);
   });
}

void addtogenesis(eosio::name new_genesis_member)
{
   auto& status = get_status();
   db.status.modify(get_status(),
                    [&](auto& obj) { obj.status.initialMembers.push_back(new_genesis_member); });
   for (auto& obj : db.inductions)
      db.inductions.modify(
          obj, [&](auto& obj) { obj.induction.witnesses.push_back(new_genesis_member); });
   add_genesis_member(status.status, new_genesis_member);
}

void inductinit(uint64_t id,
                eosio::name inviter,
                eosio::name invitee,
                std::vector<eosio::name> witnesses)
{
   // TODO: expire records

   // contract doesn't allow inductinit() until it transitioned to active
   const auto& status = get_status();
   if (!status.status.active)
      db.status.modify(status, [&](auto& obj) { obj.status.active = true; });

   add_or_replace<by_pk>(db.inductions, id, [&](auto& obj) {
      obj.induction.id = id;
      obj.induction.inviter = inviter;
      obj.induction.invitee = invitee;
      obj.induction.witnesses = witnesses;
   });
}

void inductprofil(uint64_t id, eden::new_member_profile profile)
{
   modify<by_pk>(db.inductions, id, [&](auto& obj) { obj.induction.profile = profile; });
}

void inductvideo(eosio::name account, uint64_t id, std::string video)
{
   modify<by_pk>(db.inductions, id, [&](auto& obj) { obj.induction.video = video; });
}

void inductcancel(eosio::name account, uint64_t id)
{
   remove_if_exists<by_pk>(db.inductions, id);
}

void inductdonate(eosio::name payer, uint64_t id, eosio::asset quantity)
{
   auto& induction = get<by_pk>(db.inductions, id);
   auto& member = db.members.emplace([&](auto& obj) {
      obj.member.account = induction.induction.invitee;
      obj.member.inviter = induction.induction.inviter;
      obj.member.inductionWitnesses = induction.induction.witnesses;
      obj.member.profile = induction.induction.profile;
      obj.member.inductionVideo = induction.induction.video;
   });

   auto& index = db.inductions.get<by_invitee>();
   for (auto it = index.lower_bound(std::pair<eosio::name, uint64_t>{member.member.account, 0});
        it != index.end() && it->induction.invitee == member.member.account;)
   {
      auto next = it;
      ++next;
      db.inductions.remove(*it);
      it = next;
   }
}

template <typename... Args>
void call(void (*f)(Args...), const std::vector<char>& data)
{
   std::tuple<eosio::remove_cvref_t<Args>...> t;
   eosio::input_stream s(data);
   // TODO: prevent abort, indicate what failed
   eosio::from_bin(t, s);
   std::apply([f](auto&&... args) { f(std::move(args)...); }, t);
}

void filter_block(const subchain::eosio_block& block)
{
   for (auto& trx : block.transactions)
   {
      for (auto& action : trx.actions)
      {
         if (action.firstReceiver == eden_account)
         {
            if (action.name == "clearall"_n)
               call(clearall, action.hexData.data);
            else if (action.name == "genesis"_n)
               call(genesis, action.hexData.data);
            else if (action.name == "addtogenesis"_n)
               call(addtogenesis, action.hexData.data);
            else if (action.name == "inductinit"_n)
               call(inductinit, action.hexData.data);
            else if (action.name == "inductprofil"_n)
               call(inductprofil, action.hexData.data);
            else if (action.name == "inductvideo"_n)
               call(inductvideo, action.hexData.data);
            else if (action.name == "inductcancel"_n)
               call(inductcancel, action.hexData.data);
            else if (action.name == "inductdonate"_n)
               call(inductdonate, action.hexData.data);
         }
      }
   }
}

subchain::block_log block_log;

void forked_n_blocks(size_t n)
{
   if (n)
      printf("forked %d blocks, %d now in log\n", (int)n, (int)block_log.blocks.size());
   while (n--)
      db.db.undo();
}

bool add_block(subchain::block_with_id&& bi, uint32_t eosio_irreversible)
{
   auto [status, num_forked] = block_log.add_block(bi);
   if (status)
      return false;
   forked_n_blocks(num_forked);
   if (auto* b = block_log.block_before_eosio_num(eosio_irreversible + 1))
      block_log.irreversible = std::max(block_log.irreversible, b->num);
   db.db.commit(block_log.irreversible);
   bool need_undo = bi.num > block_log.irreversible;
   auto session = db.db.start_undo_session(bi.num > block_log.irreversible);
   filter_block(bi.eosioBlock);
   session.push();
   if (!need_undo)
      db.db.set_revision(bi.num);
   // printf("%s block: %d %d log: %d irreversible: %d db: %d-%d %s\n", block_log.status_str[status],
   //        (int)bi.eosioBlock.num, (int)bi.num, (int)block_log.blocks.size(),
   //        block_log.irreversible,  //
   //        (int)db.db.undo_stack_revision_range().first,
   //        (int)db.db.undo_stack_revision_range().second,  //
   //        to_string(bi.eosioBlock.id).c_str());
   return true;
}

bool add_block(subchain::block&& eden_block, uint32_t eosio_irreversible)
{
   auto bin = eosio::convert_to_bin(eden_block);
   subchain::block_with_id bi;
   static_cast<subchain::block&>(bi) = std::move(eden_block);
   bi.id = clchain::sha256(bin.data(), bin.size());
   auto bin_with_id = eosio::convert_to_bin(bi.id);
   bin_with_id.insert(bin_with_id.end(), bin.begin(), bin.end());
   result = std::move(bin_with_id);
   return add_block(std::move(bi), eosio_irreversible);
}

// TODO: prevent from_json from aborting
[[clang::export_name("addEosioBlockJson")]] bool addEosioBlockJson(const char* json,
                                                                   uint32_t size,
                                                                   uint32_t eosio_irreversible)
{
   std::string str(json, size);
   eosio::json_token_stream s(str.data());
   subchain::eosio_block eosio_block;
   eosio::from_json(eosio_block, s);

   subchain::block eden_block;
   eden_block.eosioBlock = std::move(eosio_block);
   auto* prev = block_log.block_before_eosio_num(eden_block.eosioBlock.num);
   if (prev)
   {
      eden_block.num = prev->num + 1;
      eden_block.previous = prev->id;
   }
   else
      eden_block.num = 1;
   return add_block(std::move(eden_block), eosio_irreversible);

   // printf("%d blocks processed, %d blocks now in log\n", (int)eosio_blocks.size(),
   //        (int)block_log.blocks.size());
   // for (auto& b : block_log.blocks)
   //    printf("%d\n", (int)b->num);
}

// TODO: prevent from_bin from aborting
[[clang::export_name("addBlock")]] bool addBlock(const char* data,
                                                 uint32_t size,
                                                 uint32_t eosio_irreversible)
{
   // TODO: verify id integrity
   eosio::input_stream bin{data, size};
   subchain::block_with_id block;
   eosio::from_bin(block, bin);
   return add_block(std::move(block), eosio_irreversible);
}

[[clang::export_name("setIrreversible")]] uint32_t setIrreversible(uint32_t irreversible)
{
   if (auto* b = block_log.block_before_num(irreversible + 1))
      block_log.irreversible = std::max(block_log.irreversible, b->num);
   db.db.commit(block_log.irreversible);
   return block_log.irreversible;
}

[[clang::export_name("trimBlocks")]] void trimBlocks()
{
   block_log.trim();
}

[[clang::export_name("undoBlockNum")]] void undoBlockNum(uint32_t blockNum)
{
   forked_n_blocks(block_log.undo(blockNum));
}

[[clang::export_name("undoEosioNum")]] void undoEosioNum(uint32_t eosioNum)
{
   if (auto* b = block_log.block_by_eosio_num(eosioNum))
      forked_n_blocks(block_log.undo(b->num));
}

[[clang::export_name("getBlock")]] bool getBlock(uint32_t num)
{
   auto block = block_log.block_by_num(num);
   if (!block)
      return false;
   result = eosio::convert_to_bin(*block);
   return true;
}

constexpr const char MemberConnection_name[] = "MemberConnection";
constexpr const char MemberEdge_name[] = "MemberEdge";
using MemberConnection =
    clchain::Connection<clchain::ConnectionConfig<std::reference_wrapper<const member>,
                                                  MemberConnection_name,
                                                  MemberEdge_name>>;

struct Query
{
   subchain::BlockLog blockLog;

   MemberConnection members(std::optional<eosio::name> gt,
                            std::optional<eosio::name> ge,
                            std::optional<eosio::name> lt,
                            std::optional<eosio::name> le,
                            std::optional<uint32_t> first,
                            std::optional<uint32_t> last,
                            std::optional<std::string> before,
                            std::optional<std::string> after) const
   {
      return clchain::make_connection<MemberConnection, eosio::name>(
          gt, ge, lt, le, first, last, before, after,       //
          db.members.get<by_pk>(),                          //
          [](auto& obj) { return obj.member.account; },     //
          [](auto& obj) { return std::cref(obj.member); },  //
          [](auto& members, auto key) { return members.lower_bound(key); },
          [](auto& members, auto key) { return members.upper_bound(key); });
   }
};
EOSIO_REFLECT2(Query,
               blockLog,
               method(members, "gt", "ge", "lt", "le", "first", "last", "before", "after"))

auto schema = clchain::get_gql_schema<Query>();
[[clang::export_name("getSchemaSize")]] uint32_t getSchemaSize()
{
   return schema.size();
}
[[clang::export_name("getSchema")]] const char* getSchema()
{
   return schema.c_str();
}

[[clang::export_name("query")]] void query(const char* query,
                                           uint32_t size,
                                           const char* variables,
                                           uint32_t variables_size)
{
   Query root{block_log};
   result = clchain::gql_query(root, {query, size}, {variables, variables_size});
}