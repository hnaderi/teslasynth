import Logo from "./Logo.jsx";
import { SynthConfigSection } from "./SynthConfigSection.jsx";
import { SysInfoSection } from "./SysInfo.jsx";

export default function App() {
    return (
        <>
            <Logo size={100} />
            <SynthConfigSection />
            <SysInfoSection />
        </>
    );
}
