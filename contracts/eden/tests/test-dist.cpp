#include <tester-base.hpp>

#define CATCH_CONFIG_RUNNER

bool write_expected = false;

const eosio::private_key alice_session_priv_key =
    private_key_from_string("5KdMjZ6vrbQWromznw5v7WLt4q92abv8sKgRKzagpj8SHacnozX");
const eosio::public_key alice_session_pub_key =
    public_key_from_string("EOS665ajq1JUMwWH3bHcRxxTqiZBZBc6CakwUfLkZJxRqp4vyzqsQ");

const eosio::private_key alice_session_2_priv_key =
    private_key_from_string("5KKnoRi3WfLL82sS4WdP8YXmezVR24Y8jxy5JXzwC2SouqoHgu2");
const eosio::public_key alice_session_2_pub_key =
    public_key_from_string("EOS8VWTR1mogYHEd9HJxgG2Tj3GbPghrnJqMfWfdHbTE11BJmyLRR");

const eosio::private_key pip_session_priv_key =
    private_key_from_string("5KZLNGfDrqPM1yVL5zPXMhbAHBSi6ZtU2seqeUdEfudPgv9n93h");
const eosio::public_key pip_session_pub_key =
    public_key_from_string("EOS8YQhKe3x1xTA1KHmkBPznWqa3UGQsaHTUMkJJtcds9giK4Erft");

const eosio::private_key egeon_session_priv_key =
    private_key_from_string("5Jk9RLHvhSgN8h7VjRGdY91GpeoXs5qP7JnizReg4DXBqtbGM8y");
const eosio::public_key egeon_session_pub_key =
    public_key_from_string("EOS8kBx4XYj3zZ3Z1Sb8vdq43ursVTebfcShKMDUymiA2ctcznX71");

const eosio::private_key bertie_session_priv_key =
    private_key_from_string("5Jr4bSzJWhtr3bxY83xRDhUTgir9Mhn6YwVt4Y9SRgu1GopZ5vA");
const eosio::public_key bertie_session_pub_key =
    public_key_from_string("EOS5iALbhfqEZvqkUifUGbfMQSFnd1ui8ZsXVHT23XWh1HLyyPrJE");

int main(int argc, char* argv[])
{
   Catch::Session session;
   auto cli = session.cli() | Catch::clara::Opt(write_expected)["--write"]("Write .expected files");
   session.cli(cli);
   auto ret = session.applyCommandLine(argc, argv);
   if (ret)
      return ret;
   return session.run();
}

struct CompareFile
{
   std::string expected_path;
   std::string actual_path;
   std::ofstream file;

   CompareFile(const std::string& name)
       : expected_path("eden-test-data/" + name + ".expected"),
         actual_path("eden-test-data/" + name + ".actual"),
         file{actual_path}
   {
      eosio::check(file.is_open(), "failed to open " + actual_path);
   }

   void compare()
   {
      file.close();
      if (write_expected)
         eosio::execute("cp " + actual_path + " " + expected_path);
      else
         eosio::check(!eosio::execute("diff " + expected_path + " " + actual_path),
                      "file mismatch between " + expected_path + ", " + actual_path);
   }

   auto& write_events(eosio::test_chain& chain)
   {
      uint32_t last_block = 1;
      while (auto history = chain.get_history(last_block + 1))
      {
         for (auto& ttrace : history->traces)
         {
            std::visit(
                [&](auto& ttrace) {
                   for (auto& atrace : ttrace.action_traces)
                   {
                      std::visit(
                          [&](auto& atrace) {
                             if (atrace.receiver == "eosio.null"_n &&
                                 atrace.act.name == "eden.events"_n)
                             {
                                std::vector<eden::event> events;
                                from_bin(events, atrace.act.data);
                                for (auto& event : events)
                                {
                                   std::string str;
                                   eosio::pretty_stream<
                                       eosio::time_point_include_z_stream<eosio::string_stream>>
                                       stream{str};
                                   to_json(event, stream);
                                   file << str << "\n";
                                }
                             }
                          },
                          atrace);
                   }
                },
                ttrace);
         }
         ++last_block;
      }
      return *this;
   }  // write_events
};    // CompareFile




TEST_CASE("budget distribution")
{
   eden_tester t;
   t.genesis();
   t.set_balance(s2a("36.0000 EOS"));
   t.run_election();

   t.alice.act<actions::distribute>(250);
   CHECK(t.get_total_budget() == s2a("1.8000 EOS"));
   // Skip forward to the next distribution
   t.skip_to("2020-08-03T15:29:59.500");
   expect(t.alice.trace<actions::distribute>(250), "Nothing to do");
   t.chain.start_block();
   t.alice.act<actions::distribute>(250);
   CHECK(t.get_total_budget() == s2a("3.5100 EOS"));
   // Skip into the next election
   t.skip_to("2021-01-02T15:30:00.000");
   t.alice.act<actions::distribute>(1);
   t.alice.act<actions::distribute>(5000);
   CHECK(t.get_total_budget() == s2a("10.9435 EOS"));

   expect(t.alice.trace<actions::fundtransfer>("alice"_n, s2t("2020-07-04T15:30:00.000"), 1,
                                               "egeon"_n, s2a("1.8001 EOS"), "memo"),
          "insufficient balance");
   expect(t.alice.trace<actions::fundtransfer>("alice"_n, s2t("2020-07-04T15:30:00.000"), 1,
                                               "egeon"_n, s2a("-1.0000 EOS"), "memo"),
          "amount must be positive");
   expect(t.alice.trace<actions::fundtransfer>("alice"_n, s2t("2020-07-04T15:30:00.000"), 1,
                                               "ahab"_n, s2a("1.0000 EOS"), "memo"),
          "member ahab not found");

   t.alice.act<actions::fundtransfer>("alice"_n, s2t("2020-07-04T15:30:00.000"), 1, "egeon"_n,
                                      s2a("1.8000 EOS"), "memo");
   CHECK(get_eden_account("egeon"_n)->balance() == s2a("1.8000 EOS"));

   expect(t.alice.trace<actions::usertransfer>("alice"_n, "ahab"_n, s2a("10.0000 EOS"), "memo"),
          "member ahab not found");
   t.ahab.act<token::actions::transfer>("ahab"_n, "eden.gm"_n, s2a("10.0000 EOS"), "memo");
   expect(t.ahab.trace<actions::usertransfer>("ahab"_n, "egeon"_n, s2a("10.0000 EOS"), "memo"),
          "member ahab not found");
   expect(t.alice.trace<actions::usertransfer>("alice"_n, "egeon"_n, s2a("-1.0000 EOS"), "memo"),
          "amount must be positive");
   t.alice.act<actions::usertransfer>("alice"_n, "egeon"_n, s2a("10.0000 EOS"), "memo");
   CHECK(get_eden_account("egeon"_n)->balance() == s2a("11.8000 EOS"));
   CHECK(get_eden_account("alice"_n)->balance() == s2a("80.0000 EOS"));
   CHECK(get_eden_account("ahab"_n)->balance() == s2a("10.0000 EOS"));
}

#ifdef ENABLE_SET_TABLE_ROWS

TEST_CASE("budget distribution min")
{
   eden_tester t;
   t.genesis();
   auto members = make_names(100);
   t.create_accounts(members);
   for (auto a : members)
   {
      t.chain.start_block();
      t.induct(a);
   }
   t.set_balance(s2a("1236.0000 EOS"));
   t.run_election();
   t.set_balance(s2a("0.0020 EOS"));
   t.skip_to("2020-08-03T15:30:00.000");
   t.distribute();
   std::map<eosio::block_timestamp, eosio::asset> expected{
       {s2t("2020-07-04T15:30:00.000"), s2a("61.8000 EOS")},
       {s2t("2020-08-03T15:30:00.000"), s2a("0.0001 EOS")}};
   CHECK(t.get_budgets_by_period() == expected);
}

#endif


#ifdef ENABLE_SET_TABLE_ROWS

TEST_CASE("settablerows")
{
   eden_tester t;
   t.genesis();
   t.eden_gm.act<actions::settablerows>(
       eosio::name(eden::default_scope),
       std::vector<eden::table_variant>{
           eden::current_election_state_registration_v1{s2t("2020-01-02T00:00:00.0000")}});
   eden::current_election_state_singleton state{"eden.gm"_n, eden::default_scope};
   auto value = std::get<eden::current_election_state_registration_v1>(state.get());
   CHECK(value.start_time.to_time_point() == s2t("2020-01-02T00:00:00.0000"));
}

#endif


/*
TEST_CASE("contract-auth")
{
   eden_tester t;
   t.genesis();

   t.newsession("pip"_n, "alice"_n, alice_session_pub_key,
                t.chain.get_head_block_info().timestamp.to_time_point() + eosio::days(90),
                "no, pip, no", "missing authority of alice");
   t.newsession("alice"_n, "alice"_n, alice_session_pub_key,
                t.chain.get_head_block_info().timestamp.to_time_point(), "my first session",
                "session is expired");
   t.newsession("alice"_n, "alice"_n, alice_session_pub_key,
                t.chain.get_head_block_info().timestamp.to_time_point() + eosio::days(91),
                "my first session", "expiration is too far in the future");
   t.newsession("alice"_n, "alice"_n, alice_session_pub_key,
                t.chain.get_head_block_info().timestamp.to_time_point() + eosio::days(90),
                "four score and twenty", "description is too long");

   t.newsession("alice"_n, "alice"_n, alice_session_pub_key,
                t.chain.get_head_block_info().timestamp.to_time_point() + eosio::days(90),
                "four score and seven");
   t.newsession("alice"_n, "alice"_n, alice_session_2_pub_key,
                t.chain.get_head_block_info().timestamp.to_time_point() + eosio::days(90),
                "another session");
   t.newsession("alice"_n, "alice"_n, alice_session_2_pub_key,
                t.chain.get_head_block_info().timestamp.to_time_point() + eosio::days(60),
                "another session", "session key already exists");

   t.delsession("pip"_n, "alice"_n, alice_session_pub_key, "missing authority of alice");
   t.delsession("alice"_n, "alice"_n, alice_session_pub_key);
   t.chain.start_block();
   t.delsession("alice"_n, "alice"_n, alice_session_pub_key,
                "Session key is either expired or not found");

   t.run(alice_session_priv_key, "alice"_n, 1,
         "Recovered session key PUB_K1_665ajq1JUMwWH3bHcRxxTqiZBZBc6CakwUfLkZJxRqp4vAtJaV "
         "is either expired or not found",
         sact<actions::delsession>("alice"_n, pip_session_pub_key));
   t.run(alice_session_2_priv_key, "alice"_n, 1, "Session key is either expired or not found",
         sact<actions::delsession>("alice"_n, alice_session_pub_key));
   t.run(alice_session_2_priv_key, "alice"_n, 1, nullptr,
         sact<actions::delsession>("alice"_n, alice_session_2_pub_key));
   t.chain.start_block();
   t.run(alice_session_2_priv_key, "alice"_n, 1,
         "Recovered session key PUB_K1_8VWTR1mogYHEd9HJxgG2Tj3GbPghrnJqMfWfdHbTE11BLxqvo3 "
         "is either expired or not found",
         sact<actions::delsession>("alice"_n, alice_session_2_pub_key));

   t.write_dfuse_history("dfuse-contract-auth.json");
   CompareFile{"contract-auth"}.write_events(t.chain).compare();
}  // TEST_CASE("contract-auth")

TEST_CASE("contract-auth-induct")
{
   eden_tester t;
   t.genesis();

   t.newsession("alice"_n, "alice"_n, alice_session_pub_key,
                t.chain.get_head_block_info().timestamp.to_time_point() + eosio::days(90), "");
   t.newsession("pip"_n, "pip"_n, pip_session_pub_key,
                t.chain.get_head_block_info().timestamp.to_time_point() + eosio::days(90), "");
   t.newsession("egeon"_n, "egeon"_n, egeon_session_pub_key,
                t.chain.get_head_block_info().timestamp.to_time_point() + eosio::days(90), "");

   t.newsession("bertie"_n, "bertie"_n, bertie_session_pub_key,
                t.chain.get_head_block_info().timestamp.to_time_point() + eosio::days(90), "",
                "member bertie not found");

   t.run(pip_session_priv_key, "pip"_n, 1,
         "need authorization of alice but have authorization of pip",
         sact<actions::inductinit>(1234, "alice"_n, "bertie"_n, std::vector{"pip"_n, "egeon"_n}));
   t.run(alice_session_priv_key, "alice"_n, 1, nullptr,
         sact<actions::inductinit>(1234, "alice"_n, "bertie"_n, std::vector{"pip"_n, "egeon"_n}));
   t.newsession("bertie"_n, "bertie"_n, bertie_session_pub_key,
                t.chain.get_head_block_info().timestamp.to_time_point() + eosio::days(90), "");
   t.run(alice_session_priv_key, "alice"_n, 2,
         "need authorization of bertie but have authorization of alice",
         sact<actions::inductprofil>(1234, bertie_profile));
   t.run(bertie_session_priv_key, "bertie"_n, 1, "Video can only be set by inviter or a witness",
         sact<actions::inductprofil>(1234, bertie_profile),
         sact<actions::inductvideo>("bertie"_n, 1234, "vid"s));
   t.run(bertie_session_priv_key, "bertie"_n, 1,
         "need authorization of pip but have authorization of bertie",
         sact<actions::inductprofil>(1234, bertie_profile),
         sact<actions::inductvideo>("pip"_n, 1234, "vid"s));
   t.run(bertie_session_priv_key, "bertie"_n, 1,
         "Induction can only be endorsed by inviter or a witness",
         sact<actions::inductprofil>(1234, bertie_profile),
         sact<actions::inductendors>("bertie"_n, 1234, t.hash_induction("vid"s, bertie_profile)));
   t.run(bertie_session_priv_key, "bertie"_n, 1,
         "need authorization of pip but have authorization of bertie",
         sact<actions::inductprofil>(1234, bertie_profile),
         sact<actions::inductendors>("pip"_n, 1234, t.hash_induction("vid"s, bertie_profile)));
   t.run(bertie_session_priv_key, "bertie"_n, 1, nullptr,
         sact<actions::inductprofil>(1234, bertie_profile));

   t.run(pip_session_priv_key, "pip"_n, 2,
         "need authorization of alice but have authorization of pip",
         sact<actions::inductmeetin>("alice"_n, 1234, std::vector<eden::encrypted_key>(4),
                                     eosio::bytes{}, std::nullopt));
   t.run(pip_session_priv_key, "pip"_n, 2, nullptr,
         sact<actions::inductmeetin>("pip"_n, 1234, std::vector<eden::encrypted_key>(0),
                                     eosio::bytes{}, std::nullopt));

   t.run(pip_session_priv_key, "pip"_n, 3, nullptr,
         sact<actions::inductvideo>("pip"_n, 1234, "vid"s),
         sact<actions::inductendors>("pip"_n, 1234, t.hash_induction("vid"s, bertie_profile)));
   t.run(alice_session_priv_key, "alice"_n, 3, nullptr,
         sact<actions::inductendors>("alice"_n, 1234, t.hash_induction("vid"s, bertie_profile)));

   t.run(pip_session_priv_key, "pip"_n, 4,
         "need authorization of alice but have authorization of pip",
         sact<actions::inductcancel>("alice"_n, 1234));
   t.run(pip_session_priv_key, "pip"_n, 4, nullptr, sact<actions::inductcancel>("pip"_n, 1234));

   t.write_dfuse_history("dfuse-contract-auth-induct.json");
   CompareFile{"contract-auth-induct"}.write_events(t.chain).compare();
}  // TEST_CASE("contract-auth-induct")

TEST_CASE("contract-auth-elect")
{
   eden_tester t;
   t.genesis();
   t.induct_n(100);

   auto create_sessions = [&] {
      t.alice.trace<actions::gc>(1000);
      t.newsession("alice"_n, "alice"_n, alice_session_pub_key,
                   t.chain.get_head_block_info().timestamp.to_time_point() + eosio::days(90), "");
      t.newsession("pip"_n, "pip"_n, pip_session_pub_key,
                   t.chain.get_head_block_info().timestamp.to_time_point() + eosio::days(90), "");
      t.newsession("egeon"_n, "egeon"_n, egeon_session_pub_key,
                   t.chain.get_head_block_info().timestamp.to_time_point() + eosio::days(90), "");
   };

   create_sessions();
   t.run(pip_session_priv_key, "pip"_n, 1,
         "need authorization of alice but have authorization of pip",
         sact<actions::electopt>("alice"_n, true));
   t.run(alice_session_priv_key, "alice"_n, 1, nullptr, sact<actions::electopt>("alice"_n, true));
   t.chain.finish_block();
   t.run(alice_session_priv_key, "alice"_n, 2, "Not currently opted out",
         sact<actions::electopt>("alice"_n, true));

   t.electdonate_all();
   t.skip_to(t.next_election_time().to_time_point() - eosio::days(1));
   t.electseed(t.next_election_time().to_time_point() - eosio::days(1));
   t.skip_to(t.next_election_time().to_time_point() + eosio::minutes(10));
   t.setup_election();

   t.run(pip_session_priv_key, "pip"_n, 2,
         "Recovered session key PUB_K1_8YQhKe3x1xTA1KHmkBPznWqa3UGQsaHTUMkJJtcds9giKNsHGv "
         "is either expired or not found",
         sact<actions::electmeeting>("pip"_n, 0, std::vector<eden::encrypted_key>(0),
                                     eosio::bytes{}, std::nullopt));
   create_sessions();
   t.run(pip_session_priv_key, "pip"_n, 2,
         "need authorization of alice but have authorization of pip",
         sact<actions::electmeeting>("alice"_n, 0, std::vector<eden::encrypted_key>(0),
                                     eosio::bytes{}, std::nullopt));
   t.run(pip_session_priv_key, "pip"_n, 2, nullptr,
         sact<actions::electmeeting>("pip"_n, 0, std::vector<eden::encrypted_key>(0),
                                     eosio::bytes{}, std::nullopt));

   t.run(pip_session_priv_key, "pip"_n, 3,
         "need authorization of alice but have authorization of pip",
         sact<actions::electvote>(0, "alice"_n, "pip"_n));
   t.run(alice_session_priv_key, "alice"_n, 0, "alice and pip are not in the same group",
         sact<actions::electvote>(0, "alice"_n, "pip"_n));

   t.run(pip_session_priv_key, "pip"_n, 3,
         "need authorization of alice but have authorization of pip",
         sact<actions::electvideo>(0, "alice"_n, "Qmb7WmZiSDXss5HfuKfoSf6jxTDrHzr8AoAUDeDMLNDuws"));
   t.run(alice_session_priv_key, "alice"_n, 1, nullptr,
         sact<actions::electvideo>(0, "alice"_n, "Qmb7WmZiSDXss5HfuKfoSf6jxTDrHzr8AoAUDeDMLNDuws"));

   t.write_dfuse_history("dfuse-contract-auth-elect.json");
   CompareFile{"contract-auth-elect"}.write_events(t.chain).compare();
}  // TEST_CASE("contract-auth-elect")
*/
