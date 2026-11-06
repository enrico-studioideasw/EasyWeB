# Forum-one `toc`: oracle for the first native EWB slice

This document freezes the observable contract before the native program and
its route exist. It covers `GET ?id=TOPIC&toc` only.

## Isolation

Run each backend against an independently restored copy of the same minimal
Forum-one database fixture. The fixture contains the same user, token, topic
and contributions, and contains no `topic_reads` row for that user/topic.
It also records the initial count of `contribution_reads`.

Do not run the CGI and native arms sequentially on the same state. An upsert
in the first arm would make the second arm start from a different transition.
Database credentials belong to the node/test configuration and never to an
EWB program, request, fixture committed to Git, or expected output.

## Parity cells

| Authentication | Topic | Expected HTTP result | Expected database transition |
|---|---|---|---|
| valid Bearer | existing | `200`, index title, ordered contribution list, no form | `topic_reads`: absent to one row with `read_at` after the pre-request observation; `contribution_reads`: unchanged |
| invalid Bearer | existing | `200`, login title and password form | no change to either read table |
| absent Bearer | existing | `200`, login title and password form | no change to either read table |
| valid Bearer | missing | `404`, error title, no password form | no change to either read table |

For the valid/existing cell, compare CGI and native response status, headers
selected by the contract, and body bytes. Do not compare the exact database
timestamp between arms: verify the transition `absent -> present` and bound
`read_at` by the observation interval of its own request.

## Route provenance for every native cell

The parity result does not prove which backend produced it. A CGI replay can
return the same login or error page and leave both read tables unchanged.
Therefore run every row of the table once more with the native route selected
and the CGI fallback replaced by a marker fixture. The marker increments an
independent invocation counter before returning any response.

For all four native cells the CGI invocation counter must remain zero. This
assertion is separate from response parity and database effects: it proves
route provenance even when the expected result is a non-mutating login or
error page.

## No replay after a native side effect

Routing is selected before execution. Add a native fault point immediately
after the successful `topic_reads` upsert and before response completion.
In the same test, replace the CGI backend with a marker fixture that increments
an independent invocation counter on every call.

The request passes only if:

1. the native database contains exactly the one expected `topic_reads` row;
2. the native response is the declared native failure response;
3. the CGI invocation counter remains zero;
4. `contribution_reads` remains unchanged.

The final `topic_reads` row alone cannot prove absence of replay because two
upserts can collapse to the same row. Likewise, unchanged read tables cannot
prove that an invalid or missing request stayed native. The independent CGI
invocation counter is the route and replay oracle in every native cell.

## Gate

Do not route production traffic to the native slice until all four parity
cells, all four native route-provenance cells, and the post-upsert fault cell
pass from fresh fixtures. A native attempt is terminal for that request; CGI
is a separately selected fallback, not an error recovery replay.

In plain language: both implementations must start from the same untouched
visit state, and every native outcome must prove that CGI was never called.
