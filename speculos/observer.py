from abc import ABC, abstractmethod
from dataclasses import dataclass
from logging import Logger
from typing import List


@dataclass
class TextEvent:
    text: str
    x: int
    y: int
    w: int
    h: int
    clear: bool


class ObserverInterface(ABC):

    @abstractmethod
    def send_screen_event(self, event: TextEvent) -> None:
        pass


class BroadcastInterface(ABC):

    def __init__(self) -> None:
        self.logger: Logger
        self.clients: List[ObserverInterface] = list()

    def add_client(self, client: ObserverInterface) -> None:
        self.logger.debug("New client '%s'", client)
        self.clients.append(client)

    def remove_client(self, client: ObserverInterface) -> None:
        self.logger.debug("Removing client '%s'", client)
        self.clients.remove(client)

    @abstractmethod
    def broadcast(self, event: TextEvent) -> None:
        pass
