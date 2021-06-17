import React, { useState } from "react";

import {
    ActionButton,
    ActionButtonSize,
    Heading,
    Link,
    onError,
    Text,
    useUALAccount,
} from "_app";
import { Induction, NewMemberProfile } from "../interfaces";
import { setInductionProfileTransaction } from "../transactions";
import { InductionJourneyContainer, InductionRole } from "inductions";
import { InductionProfileForm } from "./induction-profile-form";
import { getInductionRemainingTimeDays } from "inductions/utils";

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
            onError(error, "Unable to set the profile");
        }
    };

    // Invitee profile submission confirmation
    if (submittedProfile) return <ProfileSubmitConfirmation />;

    // Invitee profile create/update form
    if (ualAccount?.accountName === induction.invitee) {
        return (
            <CreateModifyProfile
                induction={induction}
                onSubmit={submitInductionProfileTransaction}
                isReviewing={isReviewing}
            />
        );
    }

    // Inviter/endorsers profile pending screen
    return <WaitingForInviteeProfile induction={induction} />;
};

const ProfileSubmitConfirmation = () => (
    <InductionJourneyContainer role={InductionRole.INVITEE} step={3}>
        <Heading size={1} className="mb-5">
            Success!
        </Heading>
        <div className="space-y-3 mb-8">
            <Text className="leading-normal">
                Thanks for submitting your profile.
            </Text>
            <Text className="leading-normal">
                Your inviter and witnesses will be in touch to schedule the
                short video induction ceremony. Now's a good time to reach out
                to them to let them know you're ready.
            </Text>
        </div>
        <ActionButton
            onClick={() => window.location.reload()}
            size={ActionButtonSize.L}
        >
            See induction status
        </ActionButton>
    </InductionJourneyContainer>
);

interface CreateModifyProfileProps {
    induction: Induction;
    onSubmit?: (newMemberProfile: NewMemberProfile) => Promise<void>;
    isReviewing?: boolean;
}

const CreateModifyProfile = ({
    induction,
    onSubmit,
    isReviewing,
}: CreateModifyProfileProps) => (
    <InductionJourneyContainer role={InductionRole.INVITEE} step={2}>
        <Heading size={1} className="mb-2">
            {isReviewing
                ? "Review your Eden profile"
                : "Create your Eden profile"}
        </Heading>
        <Text className="mb-8">
            This invitation expires in{" "}
            {getInductionRemainingTimeDays(induction)}.
        </Text>
        <InductionProfileForm
            newMemberProfile={induction.new_member_profile}
            onSubmit={onSubmit}
        />
    </InductionJourneyContainer>
);

const WaitingForInviteeProfile = ({ induction }: { induction: Induction }) => (
    <InductionJourneyContainer role={InductionRole.INVITER} step={2}>
        <Heading size={1} className="mb-5">
            Waiting for invitee
        </Heading>
        <div className="space-y-3 mb-8">
            <Text className="leading-normal">
                We're waiting on{" "}
                <span className="font-semibold">{induction.invitee}</span> to
                set up their Eden profile.
            </Text>
            <Text className="leading-normal">
                Encourage the invitee to sign into the Membership dashboard with
                their blockchain account to complete their profile. Or you can
                share this direct link with them:
            </Text>
            <Text className="leading-normal break-all">
                <Link href={window.location.href}>{window.location.href}</Link>
            </Text>
            <Text className="leading-normal font-medium">
                This invitation expires in{" "}
                {getInductionRemainingTimeDays(induction)}.
            </Text>
        </div>
        <ActionButton href="/induction" size={ActionButtonSize.L}>
            Membership dashboard
        </ActionButton>
    </InductionJourneyContainer>
);