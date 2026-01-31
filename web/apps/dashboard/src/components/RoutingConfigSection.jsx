import { RoutingMatrix } from "./RoutingMatrix";

export function RoutingConfigSection({
    routing,
    channelCount,
    onChange
}) {
    return (
        <article>
            <header>
                <h3>MIDI Routing</h3>
                <p>MIDI channel to synth output mapping</p>
            </header>

            <label>
                <input
                    type="checkbox"
                    checked={routing.percussion}
                    role="switch"
                    onChange={e =>
                        onChange({
                            ...routing,
                            percussion: e.target.checked
                        })
                    }
                />
                Enable percussion mode
            </label>

            <hr />

            <RoutingMatrix
                mapping={routing.mapping}
                channelCount={channelCount}
                onChange={mapping =>
                    onChange({ ...routing, mapping })
                }
            />
        </article>
    );
}
