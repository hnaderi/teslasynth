import Logo from './Logo';
import { Modal } from './components/modal';

export function About({ open, onClose }) {
    return (
        <Modal
            title="About"
            open={open}
            onClose={onClose}
        >
            <Logo size={100} />
            <ul>
                <li>Author: <a href='https://hnaderi.dev'>Hossein Naderi</a></li>
                <li>Contact: <a href='mailto:mail@hnaderi.dev'>mail@hnaderi.dev</a></li>
            </ul>
        </Modal>
    );
}
