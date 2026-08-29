## Summary

<!-- What changed and why? Link master-plan phase if applicable. -->

## Validation

- [ ] `make -C tests` (host playthroughs + asset lint)
- [ ] `cd web && npm test` (if web touched)
- [ ] `cd web && npm run test:e2e` (if routes/UI/ROM flow touched)
- [ ] Refreshed `rom/thomas_bridge.gba` from CI when GBA binary changed

## Ship checklist (when relevant)

- [ ] Docs updated (`docs/changelog.md`, `docs/ship-qa.md`)
- [ ] No stacked PRs — prior open PR merged or superseded
