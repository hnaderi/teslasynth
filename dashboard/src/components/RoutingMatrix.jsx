export function RoutingMatrix({
    mapping,        // number[16]
    channelCount,   // number
    onChange
}) {
    function setRoute(midiCh, outCh) {
        const next = [...mapping];
        next[midiCh] = outCh;
        onChange(next);
    }

    function disableRoute(midiCh) {
        const next = [...mapping];
        next[midiCh] = -1;
        onChange(next);
    }

    return (
        <figure class="routing-matrix">
            <table role="grid">
                <thead>
                    <tr>
                        <th></th>
                        {Array.from({ length: channelCount }).map((_, i) => (
                            <th key={i}>Out {i + 1}</th>
                        ))}
                        <th>Off</th>
                    </tr>
                </thead>

                <tbody>
                    {mapping.map((out, midiCh) => (
                        <tr key={midiCh}>
                            <th>{midiCh == 9 ? "Percussion" : `MIDI ${midiCh + 1}`}
                            </th>

                            {Array.from({ length: channelCount }).map((_, outCh) => (
                                <td key={outCh}>
                                    <input
                                        type="radio"
                                        name={`midi-${midiCh}`}
                                        checked={out === outCh}
                                        onChange={() => setRoute(midiCh, outCh)}
                                    />
                                </td>
                            ))}

                            <td>
                                <input
                                    type="radio"
                                    name={`midi-${midiCh}`}
                                    checked={out === -1}
                                    onChange={() => disableRoute(midiCh)}
                                />
                            </td>
                        </tr>
                    ))}
                </tbody>
            </table>
        </figure>
    );
}
