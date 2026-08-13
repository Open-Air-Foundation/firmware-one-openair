# Security Policy

## Supported Versions

Security fixes are provided for the latest stable release of the AirGradient firmware and Arduino library.

| Version | Supported |
| --- | --- |
| Latest stable release | :white_check_mark: |
| Older releases | :x: |

Security fixes are not normally backported to older releases. Users should update to the latest stable release published on the [GitHub Releases](https://github.com/Open-Air-Foundation/arduino/releases) page. A GitHub release may go through staged device testing before it becomes available through automatic firmware updates.

## Reporting a Vulnerability

Please report suspected security vulnerabilities privately. **Do not open a public GitHub issue or forum topic before the vulnerability has been investigated and a fix is available.**

Email [support@airgradient.com](mailto:support@airgradient.com) with the subject:

`Security vulnerability report: Open-Air-Foundation/arduino`

Include as much of the following information as possible:

- the affected monitor model, hardware revision, firmware version, and commit or release;
- the affected component or file;
- a clear description of the vulnerability and its security impact;
- reproducible steps or a minimal proof of concept;
- relevant logs, crash output, network traces, or photographs, with credentials and personal data removed;
- any known mitigations or suggested fixes;
- how you would like to be credited, or whether you prefer to remain anonymous.

We aim to acknowledge reports within 7 calendar days and provide an initial status update within 14 calendar days. Remediation timelines depend on severity, affected hardware, and the testing required for a safe firmware rollout. If you do not receive an acknowledgement within 7 days, please follow up on the same email thread.

## Scope

This policy covers security issues in:

- firmware and library code maintained in this repository;
- firmware update and rollback mechanisms;
- local HTTP, MQTT, cloud, Wi-Fi, Bluetooth, and cellular communication paths;
- device provisioning and configuration;
- bundled or pinned dependencies when they create an exploitable issue in a supported AirGradient release.

## Coordinated Disclosure

Please allow reasonable time for investigation, development, hardware validation, and staged deployment before public disclosure. We will keep reporters informed of material progress and coordinate a disclosure date when appropriate.

When a report is accepted, we may publish a GitHub Security Advisory and release notes after a fix or mitigation is available. With the reporter's permission, we will include acknowledgement in the advisory or release notes.

## Research Guidelines

When investigating a potential vulnerability:

- test only devices and accounts that you own or are explicitly authorized to use;
- avoid accessing, modifying, or retaining other people's data;
- avoid disrupting production services or other users' devices;
- stop testing once the vulnerability and impact have been demonstrated;
- do not use social engineering, physical coercion, or denial-of-service techniques.

Good-faith reports that follow these guidelines are appreciated and will be handled constructively.
