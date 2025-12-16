import { useState } from 'preact/hooks';
import Logo from "./Logo.jsx";
import { SynthConfigSection } from "./SynthConfigSection.jsx";
import { SysInfoSection } from "./SysInfo.jsx";
import { ConfirmDialog } from './components/confirmation.jsx';
import { About } from './About.jsx';

const reboot = () => fetch('/api/sys/reboot')

export default function App() {
    const [isAboutOpened, openAbout] = useState(false);
    const [confirm, requestConfirm] = useState(null);
    function onReboot() {
        requestConfirm({
            title: "Confirm reboot",
            message: "This will restart the device, are you sure?",
            onConfirm: reboot,
        })
    }

    return (
        <>
            <nav>
                <ul>
                    <Logo size={100} />
                </ul>
                <ul>
                    <li><a href="#" onClick={() => openAbout(true)}>About</a></li>
                    <li><a href="#" onClick={onReboot}>Reboot</a></li>
                </ul>
            </nav>
            <SynthConfigSection requestConfirm={requestConfirm} />
            <SysInfoSection />
            <ConfirmDialog
                open={!!confirm}
                title={confirm?.title}
                message={confirm?.message}
                onConfirm={() => {
                    confirm.onConfirm();
                    requestConfirm(null);
                }}
                onCancel={() => requestConfirm(null)}
            />
            <About
                open={isAboutOpened}
                onClose={() => openAbout(false)}
            />

        </>
    );
}
