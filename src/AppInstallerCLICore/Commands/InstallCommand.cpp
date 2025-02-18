// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "InstallCommand.h"
#include "Workflows/CompletionFlow.h"
#include "Workflows/InstallFlow.h"
#include "Workflows/WorkflowBase.h"
#include "Resources.h"

using namespace AppInstaller::CLI::Execution;
using namespace AppInstaller::CLI::Workflow;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::Utility::literals;

namespace AppInstaller::CLI
{
    namespace
    {
        constexpr Utility::LocIndView s_ArgumentName_Scope = "scope"_liv;
        constexpr Utility::LocIndView s_ArgumentName_Architecture = "architecture"_liv;
    }

    std::vector<Argument> InstallCommand::GetArguments() const
    {
        return {
            Argument::ForType(Args::Type::Query),
            Argument::ForType(Args::Type::Manifest),
            Argument::ForType(Args::Type::Id),
            Argument::ForType(Args::Type::Name),
            Argument::ForType(Args::Type::Moniker),
            Argument::ForType(Args::Type::Version),
            Argument::ForType(Args::Type::Channel),
            Argument::ForType(Args::Type::Source),
            Argument{ s_ArgumentName_Scope, Argument::NoAlias, Args::Type::InstallScope, Resource::String::InstallScopeDescription, ArgumentType::Standard, Argument::Visibility::Help },
            Argument{ s_ArgumentName_Architecture, 'a', Args::Type::InstallArchitecture, Resource::String::InstallArchitectureArgumentDescription, ArgumentType::Standard, Argument::Visibility::Help},
            Argument::ForType(Args::Type::Exact),
            Argument::ForType(Args::Type::Interactive),
            Argument::ForType(Args::Type::Silent),
            Argument::ForType(Args::Type::Locale),
            Argument::ForType(Args::Type::Log),
            Argument::ForType(Args::Type::Override),
            Argument::ForType(Args::Type::InstallLocation),
            Argument::ForType(Args::Type::HashOverride),
            Argument::ForType(Args::Type::DependencySource),
            Argument::ForType(Args::Type::AcceptPackageAgreements),
            Argument::ForType(Args::Type::CustomHeader),
            Argument::ForType(Args::Type::AcceptSourceAgreements),
        };
    }

    Resource::LocString InstallCommand::ShortDescription() const
    {
        return { Resource::String::InstallCommandShortDescription };
    }

    Resource::LocString InstallCommand::LongDescription() const
    {
        return { Resource::String::InstallCommandLongDescription };
    }

    void InstallCommand::Complete(Context& context, Args::Type valueType) const
    {
        switch (valueType)
        {
        case Args::Type::Query:
        case Args::Type::Manifest:
        case Args::Type::Id:
        case Args::Type::Name:
        case Args::Type::Moniker:
        case Args::Type::Version:
        case Args::Type::Channel:
        case Args::Type::Source:
            context <<
                Workflow::CompleteWithSingleSemanticsForValue(valueType);
            break;
        case Args::Type::Locale:
            // May well move to CompleteWithSingleSemanticsForValue,
            // but for now output nothing.
            context <<
                Workflow::CompleteWithEmptySet;
            break;
        case Args::Type::Log:
        case Args::Type::InstallLocation:
            // Intentionally output nothing to allow pass through to filesystem.
            break;
        }
    }

    std::string InstallCommand::HelpLink() const
    {
        return "https://aka.ms/winget-command-install";
    }

    void InstallCommand::ValidateArgumentsInternal(Args& execArgs) const
    {
        if (execArgs.Contains(Args::Type::Manifest) &&
            (execArgs.Contains(Args::Type::Query) ||
             execArgs.Contains(Args::Type::Id) ||
             execArgs.Contains(Args::Type::Name) ||
             execArgs.Contains(Args::Type::Moniker) ||
             execArgs.Contains(Args::Type::Version) ||
             execArgs.Contains(Args::Type::Channel) ||
             execArgs.Contains(Args::Type::Source) ||
             execArgs.Contains(Args::Type::Exact)))
        {
            throw CommandException(Resource::String::BothManifestAndSearchQueryProvided);
        }

        if (execArgs.Contains(Args::Type::InstallScope))
        {
            if (ConvertToScopeEnum(execArgs.GetArg(Args::Type::InstallScope)) == Manifest::ScopeEnum::Unknown)
            {
                throw CommandException(Resource::String::InvalidArgumentValueError, s_ArgumentName_Scope, { "user"_lis, "machine"_lis });
            }
        }
        if (execArgs.Contains(Args::Type::InstallArchitecture))
        {
	        Utility::Architecture selectedArch = Utility::ConvertToArchitectureEnum(std::string(execArgs.GetArg(Args::Type::InstallArchitecture)));
            if ((selectedArch == Utility::Architecture::Unknown) || (Utility::IsApplicableArchitecture(selectedArch) == Utility::InapplicableArchitecture))
            {
                std::vector<Utility::LocIndString> applicableArchitectures;
                for (Utility::Architecture i : Utility::GetApplicableArchitectures())
                {
                    applicableArchitectures.emplace_back(Utility::ToString(i));
                }
                throw CommandException(Resource::String::InvalidArgumentValueError, s_ArgumentName_Architecture, std::forward<std::vector<Utility::LocIndString>>((applicableArchitectures)));
            }
        }

        if (execArgs.Contains(Args::Type::Locale))
        {
            if (!Locale::IsWellFormedBcp47Tag(execArgs.GetArg(Args::Type::Locale)))
            {
                throw CommandException(Resource::String::InvalidArgumentValueErrorWithoutValidValues, Argument::ForType(Args::Type::Locale).Name(), {});
            }
        }
    }

    void InstallCommand::ExecuteInternal(Context& context) const
    {
        context.SetFlags(ContextFlag::ShowSearchResultsOnPartialFailure);

        context <<
            Workflow::ReportExecutionStage(ExecutionStage::Discovery) <<
            Workflow::GetManifest <<
            Workflow::SelectInstaller <<
            Workflow::EnsureApplicableInstaller <<
            Workflow::InstallSinglePackage;
    }
}
