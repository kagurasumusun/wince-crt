# GitHub Authentication

A GitHub Personal Access Token (PAT) is provided below because this agent environment does not support environment variables or secret storage.

```text
GITHUB_TOKEN=<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>
```

When performing GitHub API, GitHub CLI, repository-management, or other authenticated GitHub operations:

* Use the PAT provided above for authentication.
* Treat the PAT as a secret credential.
* Never print the PAT or its full value in terminal output, logs, responses, patches, commits, or generated files.
* Never commit the PAT to any file other than this `AGENT.md`.
* Never copy the PAT into source code, configuration files, documentation, issue comments, pull requests, or repository files.
* Do not expose the PAT in error messages or debugging output.
* Use only the minimum GitHub permissions necessary for the requested operation.
* Do not perform unrelated GitHub operations using this credential.
* If authentication fails, report the failure without displaying the token.
* Do not attempt to obtain, modify, or replace the PAT from any other source.

The PAT in this file is intentionally provided as a credential for this agent environment. Do not ask the user to provide it again unless the credential is absent or invalid.
