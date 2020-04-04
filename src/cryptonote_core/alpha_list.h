// Copyright (c)      2020, The Coinevo Project
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include "blockchain.h"

#define ROLLBACK_EVENT_EXPIRATION_BLOCKS 30

namespace alphas
{
  class alpha_list
    : public cryptonote::Blockchain::BlockAddedHook,
      public cryptonote::Blockchain::BlockchainDetachedHook,
      public cryptonote::Blockchain::InitHook,
      public cryptonote::Blockchain::ValidateMinerTxHook
  {
  public:
    alpha_list(cryptonote::Blockchain& blockchain);
    void block_added(const cryptonote::block& block, const std::vector<cryptonote::transaction>& txs);
    void blockchain_detached(uint64_t height);
    void init();
    bool validate_miner_tx(const crypto::hash& prev_id, const cryptonote::transaction& miner_tx, uint64_t height, int hard_fork_version, uint64_t base_reward);

    std::vector<cryptonote::account_public_address> get_expired_nodes(uint64_t block_height);
    cryptonote::account_public_address select_winner(const crypto::hash& prev_id);

    std::vector<cryptonote::account_public_address> get_alphas_pubkeys();

  private:
    bool process_registration_tx(const cryptonote::transaction& tx, uint64_t block_height, cryptonote::account_public_address& address);
    template<typename T>
    void block_added_generic(const cryptonote::block& block, const T& txs);

    bool reg_tx_has_correct_unlock_time(const cryptonote::transaction& tx, uint64_t block_height) const;
    bool reg_tx_extract_fields(const cryptonote::transaction& tx, cryptonote::account_public_address& address, crypto::public_key& tx_pub_key) const;
    bool is_reg_tx_alpha_output(const cryptonote::transaction& tx, int i, uint64_t block_height, crypto::key_derivation derivation, hw::device& hwdev) const;

    cryptonote::account_public_address find_alpha_from_miner_tx(const cryptonote::transaction& miner_tx, uint64_t block_height) const;

    class rollback_event
    {
    public:
      rollback_event(uint64_t block_height);
      virtual ~rollback_event() { }
      virtual bool apply(std::unordered_map<cryptonote::account_public_address, std::pair<uint64_t, size_t>>& alphas_last_reward) const = 0;
      uint64_t m_block_height;
    };

    class rollback_change : public rollback_event
    {
    public:
      rollback_change(uint64_t block_height, cryptonote::account_public_address address, std::pair<uint64_t, size_t> height_index);
      bool apply(std::unordered_map<cryptonote::account_public_address, std::pair<uint64_t, size_t>>& alphas_last_reward) const;
    private:
      cryptonote::account_public_address m_address;
      std::pair<uint64_t, size_t> m_height_index;
    };

    class rollback_new : public rollback_event
    {
    public:
      rollback_new(uint64_t block_height, cryptonote::account_public_address address);
      bool apply(std::unordered_map<cryptonote::account_public_address, std::pair<uint64_t, size_t>>& alphas_last_reward) const;
    private:
      cryptonote::account_public_address m_address;
    };

    class prevent_rollback : public rollback_event
    {
    public:
      prevent_rollback(uint64_t block_height);
      bool apply(std::unordered_map<cryptonote::account_public_address, std::pair<uint64_t, size_t>>& alphas_last_reward) const;
    };

    // Service nodes are organized by time since last reward or registration
    // This value is given by block height, and differentiated by transaction index for
    // registrations that occured in the same block. index = 0 for block reward, 1 for first transaction, etc.
    // hence a std::pair<uint64_t, size_t> is used here for this value.
    std::unordered_map<cryptonote::account_public_address, std::pair<uint64_t, size_t>> m_alphas_last_reward;
    std::list<std::unique_ptr<rollback_event>> m_rollback_events;
    cryptonote::Blockchain& m_blockchain;

  };

  const static cryptonote::account_public_address null_address{ crypto::null_pkey, crypto::null_pkey };
}
