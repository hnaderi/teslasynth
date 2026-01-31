import { useState } from 'preact/hooks';
import { Logo } from "@teslasynth/ui/components/Logo";

export default function App() {
    const [isAboutOpened, openAbout] = useState(false);
    const [isRebootOpen, openReboot] = useState(false);

    return (
        <>
            <nav>
                <ul>
                    <Logo size={100} />
                </ul>
                <ul>
                    <li><a href="#" onClick={() => openAbout(true)}>About</a></li>
                    <li><a href="#" onClick={() => openReboot(true)}>Reboot</a></li>
                </ul>
            </nav>


        </>
    );
}
