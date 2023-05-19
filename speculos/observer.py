from abc import ABC, abstractmethod

from .struct import TextEvent


class ObserverInterface(ABC):

    @abstractmethod
    def send_screen_event(self, event: TextEvent) -> None:
        pass


class BroadcastInterface(ABC):

    @abstractmethod
    def add_client(self, client: ObserverInterface) -> None:
        pass

    @abstractmethod
    def remove_client(self, client: ObserverInterface) -> None:
        pass

    @abstractmethod
    def broadcast(self, event: TextEvent) -> None:
        pass
