import { useState } from "react";

import { Link, Text, useUALAccount } from "_app";
import { Induction, NewMemberProfile } from "../interfaces";
import { setInductionProfileTransaction } from "../transactions";
import { InductionProfileForm } from "./induction-profile-form";

interface Props {
    induction: Induction;
    isReviewing?: boolean;
}

export const InductionStepProfile = ({ induction, isReviewing }: Props) => {
    const [ualAccount] = useUALAccount();

    const [submittedProfile, setSubmittedProfile] = useState(false);

    const submitInductionProfileTransaction = async (
        newMemberProfile: NewMemberProfile
    ) => {
        try {
            const authorizerAccount = ualAccount.accountName;
            const transaction = setInductionProfileTransaction(
                authorizerAccount,
                induction.id,
                newMemberProfile
            );
            console.info(transaction);
            const signedTrx = await ualAccount.signTransaction(transaction, {
                broadcast: true,
            });
            console.info("inductprofil trx", signedTrx);
            setSubmittedProfile(true);
        } catch (error) {
            console.error(error);
            alert(
                "Error while initializing the induction process: " +
                    JSON.stringify(error)
            );
        }
    };

    return (
        <>
            {submittedProfile ? (
                <div>
                    <Text className="mb-4">
                        Thanks for submitting your profile!
                    </Text>
                    <Link onClick={() => window.location.reload()}>
                        Click here to refresh the page and view your induction
                        process status.
                    </Link>
                </div>
            ) : (
                <>
                    <div className="text-lg mb-4 text-gray-900">
                        {isReviewing
                            ? "Reviewing Profile"
                            : "Step 1/3: Pending New Member Profile"}
                    </div>
                    {ualAccount?.accountName === induction.invitee ? (
                        <InductionProfileForm
                            newMemberProfile={induction.new_member_profile}
                            onSubmit={submitInductionProfileTransaction}
                        />
                    ) : (
                        <div>
                            Waiting for profile submission from{" "}
                            {induction.invitee}...
                        </div>
                    )}
                </>
            )}
        </>
    );
};
