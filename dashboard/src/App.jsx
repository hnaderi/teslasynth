import { useState } from 'preact/hooks';
import Logo from "./Logo.jsx";
import { SynthConfigSection } from "./SynthConfigSection.jsx";
import { ConfirmDialog } from './components/confirmation';
import { SysInfoSection } from "./SysInfo.jsx";
import { About } from './About.jsx';
import { HardwareConfigSection } from './components/HardwareConfigSection.jsx';

function RebootingScreen() {
    return (
        <article>
            <h2>Rebooting device…</h2>
            <p>
                The device will exit maintenance mode and is temporarily unavailable over Wi-Fi.
            </p>
            <p>
                If everything goes correctly, the device will restart normally and Wi-Fi will be offline.
                If there is an issue, it will return to maintenance mode and you can try again.
            </p>
            <p>
                Please wait a few moments before taking further action.
            </p>
        </article>
    );
}

export default function App() {
    const [isAboutOpened, openAbout] = useState(false);
    const [isRebootOpen, openReboot] = useState(false);
    const [isAvailable, setAvailability] = useState(true);

    async function reboot() {
        fetch('/api/sys/reboot', { method: 'POST' });
        setAvailability(false);
    }

    if (!isAvailable) return RebootingScreen()
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
            <ConfirmDialog
                open={isRebootOpen}
                title="Confirm reboot"
                message="This will restart the device, are you sure?"
                busy={false}
                onCancel={() => openReboot(false)}
                onConfirm={reboot}
            />
            <SynthConfigSection />
            <HardwareConfigSection />
            <SysInfoSection />
            <About
                open={isAboutOpened}
                onClose={() => openAbout(false)}
            />

        </>
    );
}
