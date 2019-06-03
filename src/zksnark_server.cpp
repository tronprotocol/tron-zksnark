#include <sys/timeb.h>

#include "zksnark_server.h"
#include "librustzcash.h"
#include "easylogging++.h"

using google::protobuf::Any;
using protocol::ShieldedTransferContract;
using protocol::SpendDescription;
using protocol::ReceiveDescription;

long cost(struct timeb startTime, struct timeb endTime) {
    return (endTime.time - startTime.time) * 1000 + (endTime.millitm - startTime.millitm);
}

Status TronZksnarkServiceImpl::CheckZksnarkProof(ServerContext *context, const ZksnarkRequest *request, ZksnarkResponse *reply) {
    struct timeb startAllTime , endAllTime, spendStartTime, spendEndTime, outputStartTime, outputEndTime, finalStartTime, finalEndTime;
    ftime(&startAllTime);
    Transaction transaction = request->transaction();
    const char *sighash = request->sighash().c_str();
    ShieldedTransferContract contract;
    if (!transaction.raw_data().contract(0).parameter().UnpackTo(&contract)) {
        LOG(ERROR) << "contract unpack error. txId: " << request->txid();
        reply->set_code(ZksnarkResponse::FAILED);
        return Status::CANCELLED;
    }

    if (contract.spend_description().empty() && contract.receive_description().empty()) {
        LOG(ERROR) << "spend_description and receive_description are emtpy. txId: " << request->txid();
        reply->set_code(ZksnarkResponse::FAILED);
        return Status::CANCELLED;
    }

    auto ctx = librustzcash_sapling_verification_ctx_init();
    ftime(&spendStartTime);
    for (const SpendDescription& spendDescription : contract.spend_description()) {
        if (!librustzcash_sapling_check_spend(ctx,
            reinterpret_cast<const unsigned char *>(spendDescription.value_commitment().c_str()),
            reinterpret_cast<const unsigned char *>(spendDescription.anchor().c_str()),
            reinterpret_cast<const unsigned char *>(spendDescription.nullifier().c_str()),
            reinterpret_cast<const unsigned char *>(spendDescription.rk().c_str()),
            reinterpret_cast<const unsigned char *>(spendDescription.zkproof().c_str()),
            reinterpret_cast<const unsigned char *>(spendDescription.spend_authority_signature().c_str()),
            reinterpret_cast<const unsigned char *>(sighash)
            )) {
            LOG(ERROR) << "librustzcash_sapling_check_spend failed. txId: " << request->txid();
            reply->set_code(ZksnarkResponse::FAILED);
            return Status::CANCELLED;
        }
    }
    ftime(&spendEndTime);

    ftime(&outputStartTime);
    for (const ReceiveDescription& receiveDescription : contract.receive_description()) {
        if (!librustzcash_sapling_check_output(ctx,
            reinterpret_cast<const unsigned char *>(receiveDescription.value_commitment().c_str()),
            reinterpret_cast<const unsigned char *>(receiveDescription.note_commitment().c_str()),
            reinterpret_cast<const unsigned char *>(receiveDescription.epk().c_str()),
            reinterpret_cast<const unsigned char *>(receiveDescription.zkproof().c_str())
            )) {
            LOG(ERROR) << "librustzcash_sapling_check_output failed. txId: " << request->txid();
            reply->set_code(ZksnarkResponse::FAILED);
            return Status::CANCELLED;
        }
    }
    ftime(&outputEndTime);

    ftime(&finalStartTime);
    if (!librustzcash_sapling_final_check(ctx,
        request->valuebalance(),
        reinterpret_cast<const unsigned char *>(contract.binding_signature().c_str()),
        reinterpret_cast<const unsigned char *>(sighash)
    )) {
        LOG(ERROR) << "librustzcash_sapling_final_check failed. txId: " << request->txid();
        reply->set_code(ZksnarkResponse::FAILED);
        return Status::CANCELLED;
    }
    ftime(&finalEndTime);

    reply->set_code(ZksnarkResponse::SUCCESS);
    ftime(&endAllTime);
    LOG(INFO) << "check successfully. txId: " << request->txid()
    << ", cost: " << cost(startAllTime, endAllTime)
    << ", spend cost:" << cost(spendStartTime, spendEndTime)
    << ", output cost:" << cost(outputStartTime, outputEndTime)
    << ", final cost:" << cost(finalStartTime, finalEndTime);

    LOG(INFO) << "d:" << std::boolalpha << librustzcash_check_diversifier(reinterpret_cast<const unsigned char *>(request->d().c_str()));

    return Status::OK;
}
