# Futureverse UBF Controller - Unreal

**A Futureverse UBF Orchestration layer for Unreal, by [Futureverse](https://www.futureverse.com)**

The **Futureverse UBF Controller** is a Unreal Engine plugin that acts as an orchestration layer for the [UBF Interpreter](https://github.com/futureversecom/ubf-unreal-plugin). It integrates tightly with other Futureverse systems—like the Asset Register, Sylo, and FuturePass—to dynamically resolve assets, metadata, and artifacts, and to sequence execution of parsing and rendering blueprints.

> For more information about the Universal Blueprint Framework, its authoring tools, and the full ecosystem, please visit the [UBF Open Standard](https://ubfstandard.com/) and the [Futureverse Developer Documentation](https://docs.futureverse.com/1134b651-6817-4acb-ab1a-7bced4b15e80).

---

## Installation

Download latest release and extract the zip into your unreal project's plugin folder.

## Overview

The Futureverse UBF Controller acts as an orchestration layer on top of the [UBF Interpreter](https://github.com/futureversecom/ubf-unreal-plugin). It is responsible for setting up the execution context that the Interpreter requires to function effectively. While the Interpreter is responsible for translating and running the Blueprint logic itself, the FutureverseUBFControllerSubsystem determines what Blueprint to run, with what data, in what order, and under what context.

The FutureverseUBFControllerSubsystem bridges the gap between high-level domain concepts and low-level blueprint execution. They abstract the complexity of managing inputs, resources, dependencies, and orchestration, leaving the interpreter to focus on what it does best: executing Blueprints.
​
## Core Responsibilities

FutureverseUBFControllerSubsystem can serve a wide variety of scenarios, and its responsibilities depend on the particular use case. At a fundamental level, FutureverseUBFControllerSubsystem is responsible for ensuring that everything is properly prepared before blueprint execution begins. Think of the controller as the system that packages and resolves all dependencies, ensuring the interpreter has a complete and coherent context to operate within.

This FutureverseUBFControllerSubsystem is designed specifically with Futureverse domain workflows in mind, including the handling of NFT metadata, complex asset trees (via the Asset Register), and asset profiles. Some of these responsibilities include, but are not limited to:

* Resolving Asset Profiles: Retrieve, deserialize, and interpret asset profiles in order to resolve the correct version and variation of a given asset, including the parsing/render blueprints and artifact catalogs that accompany it.
* Managing Asset Trees: Understand and resolve tree-structured relationships from Asset Register, where one asset may have other assets equipped onto it.
* Metadata Parsing & Blueprint Instantiation: Execute parsing blueprints to interpret raw metadata and feed the output into rendering blueprints as inputs. This chaining allows dynamic transformation of inputs prior to rendering.
* Artifact Provision: Provide the ability to download external artifacts through HTTPS or Decentralized Identifiers (DIDs), enabling compatibility with systems like Futureverse’s Sylos.
* Execution Ordering: Coordinate and sequence multiple blueprint executions across various entities to achieve a desired final result (e.g., rendering a full NFT asset with all attachments).

## Use Case: NFT Rendering

As mentioned above, this Execution Controller is designed primarily for rendering NFTs from metadata and leveraging other Futureverse technology. Here is how a typical flow works:

* Receive Asset Tree from Experience: The asset tree—representing the full hierarchy of a target NFT and any attached or equipped assets—is passed into the execution controller by the surrounding application or experience.
* Resolve Asset Profiles: For each asset in the tree, retrieve and load the corresponding asset profile to then evaluate the appropriate version, variant, blueprints, and catalogs.
* Download Blueprints and Catalogs: Download and cache all relevant blueprints and artifact catalogs needed to perform the rendering operation.
* Parse Metadata: Where applicable, take raw metadata or traits associated with the asset and pass it into the parsing blueprint.
* Feed Parsed Inputs into Render Blueprint: Collect the outputs from the parsing blueprint and feed them into the inputs of the rendering blueprint.
* Execute Render Blueprint: Initiate execution of the top-level asset’s render blueprint, now fully configured with its required inputs.
* Provide Artifacts at Runtime: During execution, fulfill any artifact requests from the blueprint(s) by retrieving them from cache or downloading them on demand.

## Using the Execution Controller

* Add UBFRuntimeController to actor
* Query Asset Registry for items using ArInventoryComponent
* Select which item to render
* Call RenderItem on FutureverseUBFControllerSubsystem and provide the desired item and target UBFRuntimeController
